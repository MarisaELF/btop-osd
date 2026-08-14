#include "osdengine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "btop_shared.hpp"
#include "btop_config.hpp"

//? The btop collectors keep their current state in namespace-local variables
//? that are defined in linux/btop_collect.cpp but not declared in the headers;
//? declare them here so the OSD engine can read the collected data directly.
namespace Cpu {
	extern cpu_info current_cpu;
}
namespace Mem {
	extern mem_info current_mem;
}
namespace Gpu {
	extern std::vector<gpu_info> gpus;
}
namespace Proc {
	extern std::vector<proc_info> current_procs;
}

//? ------------------------------------------------------------------ helpers

static QString humanBytes(double bytes, bool useBinary) {
    static const char* unitsBin[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    static const char* unitsDec[] = {"B", "KB", "MB", "GB", "TB"};
    const auto* units = useBinary ? unitsBin : unitsDec;
    const double base = useBinary ? 1024.0 : 1000.0;
    int u = 0;
    while (bytes >= base && u < 4) { bytes /= base; ++u; }
    return QString::asprintf("%.1f %s", bytes, units[u]);
}

static QString humanSpeed(double bytesPerSec) {
    return humanBytes(bytesPerSec, true) + "/s";
}

static QString procMemSize(double bytes) {
    if (bytes >= 1024.0 * 1024.0 * 1024.0)
        return QString::asprintf("%.2fGB", bytes / (1024.0 * 1024.0 * 1024.0));
    return QString::asprintf("%.1fMB", bytes / (1024.0 * 1024.0));
}

static int cpuHzToMHz(const std::string& hz) {
    // btop reports e.g. "3.40GHz", "800MHz" or "".
    if (hz.empty()) return 0;
    double v = 0.0;
    char unit = 0;
    if (std::sscanf(hz.c_str(), "%lf%cHz", &v, &unit) != 2 && std::sscanf(hz.c_str(), "%lf%c", &v, &unit) != 2)
        return 0;
    if (unit == 'G' || unit == 'g') return static_cast<int>(v * 1000);
    if (unit == 'M' || unit == 'm') return static_cast<int>(v);
    return 0;
}

static int threadFreqMHz(int cpu, int fallback) {
    char path[128];
    std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", cpu);
    std::ifstream f(path);
    long long khz = 0;
    f >> khz;
    if (khz > 0) return static_cast<int>(khz / 1000);
    return fallback;
}

//* Logical cpu index -> physical core index, parsed once from /proc/cpuinfo.
static const std::vector<int>& threadToCore() {
    static std::vector<int> mapping;
    if (!mapping.empty()) return mapping;
    std::ifstream in("/proc/cpuinfo");
    std::string line;
    int processor = -1;
    int coreId = 0;
    std::map<int, int> cpuToCore;
    while (std::getline(in, line)) {
        if (line.rfind("processor", 0) == 0) {
            if (auto pos = line.find(':'); pos != std::string::npos)
                processor = std::stoi(line.substr(pos + 1));
        } else if (line.rfind("core id", 0) == 0) {
            if (auto pos = line.find(':'); pos != std::string::npos)
                coreId = std::stoi(line.substr(pos + 1));
        } else if (line.empty() && processor >= 0) {
            cpuToCore[processor] = coreId;
            processor = -1;
        }
    }
    if (processor >= 0) cpuToCore[processor] = coreId;

    std::map<int, int> coreToGroup;
    int nextGroup = 0;
    for (const auto& [cpu, core] : cpuToCore) {
        auto it = coreToGroup.find(core);
        if (it == coreToGroup.end()) coreToGroup[core] = nextGroup++;
        mapping.push_back(coreToGroup[core]);
    }
    return mapping;
}

//? ------------------------------------------------------------------ OsdEngine

void OsdEngine::init() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    Shared::init();
    Net::collect();
}

QStringList OsdEngine::interfaces() {
    QStringList list;
    for (const auto& iface : Net::interfaces) list << QString::fromStdString(iface);
    return list;
}

QStringList OsdEngine::gpuNames() {
    QStringList list;
    for (const auto& name : Gpu::gpu_names) list << QString::fromStdString(name);
    return list;
}

void OsdEngine::collect(const OsdConfig& cfg) {
    Cpu::collect();
    Mem::collect();
    Net::collect();
    if (cfg.showTopProc) Proc::collect();
    if (!Gpu::gpus.empty()) Gpu::collect();
}

QVector<OsdLine> OsdEngine::buildLines(const OsdConfig& cfg) {
    QVector<OsdLine> out;

    auto addHeader = [&out](const QString& text, const QColor& color) {
        out.push_back({text, color, true});
    };
    auto addRow = [&out](const QString& text, const QColor& color) {
        out.push_back({text, color, false});
    };

    //? ------------------------------------------------ CPU
    if (cfg.showCpu) {
        const auto& cpu = Cpu::current_cpu;
        const int totalThreads = static_cast<int>(cpu.core_percent.size());
        QString model = QString::fromStdString(Cpu::cpuName);
        if (model.isEmpty()) model = "CPU";
        addHeader("CPU " + model, cfg.cpuColor);

        const int avgFreqMHz = cpuHzToMHz(Cpu::cpuHz);
        if (totalThreads > 0) {
            const auto& coreMap = threadToCore();
            struct Unit { int index; double load; int freq; };
            std::vector<Unit> units;

            if (cfg.cpuByThread) {
                units.reserve(totalThreads);
                for (int i = 0; i < totalThreads; ++i) {
                    if (cpu.core_percent[i].empty()) continue;
                    units.push_back({i, static_cast<double>(cpu.core_percent[i].back()),
                                     threadFreqMHz(i, avgFreqMHz)});
                }
            } else {
                int maxGroup = 0;
                for (int i = 0; i < totalThreads && i < static_cast<int>(coreMap.size()); ++i)
                    maxGroup = std::max(maxGroup, coreMap[i] + 1);
                std::vector<double> sumLoad(maxGroup, 0.0);
                std::vector<long long> sumFreq(maxGroup, 0);
                std::vector<int> count(maxGroup, 0);
                for (int i = 0; i < totalThreads; ++i) {
                    const int g = (i < static_cast<int>(coreMap.size())) ? coreMap[i] : (i % maxGroup);
                    if (cpu.core_percent[i].empty()) continue;
                    sumLoad[g] += static_cast<double>(cpu.core_percent[i].back());
                    sumFreq[g] += threadFreqMHz(i, avgFreqMHz);
                    count[g]++;
                }
                for (int g = 0; g < maxGroup; ++g) {
                    if (count[g] == 0) continue;
                    units.push_back({g, sumLoad[g] / count[g],
                                     static_cast<int>(sumFreq[g] / count[g])});
                }
            }

            const QString labelWord = (!cfg.cpuByThread || cfg.forceCoreLabel) ? "Core" : "Thread";
            QString row;
            int col = 0;
            for (const auto& u : units) {
                QString item;
                item += QString("%1 %2").arg(labelWord)
                            .arg(u.index, cfg.cpuByThread && !cfg.forceCoreLabel ? 4 : 2, 10, QChar('0'));
                if (cfg.showCpuFreq)
                    item += QString::asprintf("%5dMHz ", u.freq);
                if (cfg.showCpuUsage)
                    item += QString::asprintf("%4.0f%%", std::min(100.0, u.load));
                item += "    ";
                row += item;
                if (++col >= cfg.cpuCols) {
                    addRow(row.trimmed(), cfg.cpuColor);
                    row.clear();
                    col = 0;
                }
            }
            if (!row.trimmed().isEmpty()) addRow(row.trimmed(), cfg.cpuColor);
        }
    }

    //? ------------------------------------------------ GPU
    const int gpuCount = static_cast<int>(Gpu::gpus.size());
    for (int i = 0; i < gpuCount; ++i) {
        if (i >= cfg.gpuShow.size() || !cfg.gpuShow[i]) continue;
        const auto& gpu = Gpu::gpus[i];
        const bool showUsage = i < cfg.gpuShowUsage.size() && cfg.gpuShowUsage[i];
        const bool showVram = i < cfg.gpuShowVram.size() && cfg.gpuShowVram[i];
        const bool showVramRate = i < cfg.gpuShowVramRate.size() && cfg.gpuShowVramRate[i];

        QString name = i < static_cast<int>(Gpu::gpu_names.size()) && !Gpu::gpu_names[i].empty()
                           ? QString::fromStdString(Gpu::gpu_names[i])
                           : QString("GPU %1").arg(i);
        addHeader(name, cfg.gpuColor);

        QString text;
        if (showUsage) {
            if (gpu.supported_functions.gpu_utilization && !gpu.gpu_percent.at("gpu-totals").empty())
                text += QString::asprintf("GPU %3.0f%%    ", static_cast<double>(gpu.gpu_percent.at("gpu-totals").back()));
            else
                text += "GPU N/A    ";
        }
        if (showVram || showVramRate) {
            if (gpu.supported_functions.mem_used || gpu.supported_functions.mem_total) {
                const double usedMB = gpu.mem_used / (1024.0 * 1024.0);
                const double totalMB = gpu.mem_total / (1024.0 * 1024.0);
                const double pct = (gpu.mem_total > 0) ? (gpu.mem_used * 100.0 / gpu.mem_total) : 0.0;
                if (showVram && showVramRate)
                    text += QString::asprintf("VRAM %.0fMB / %.0fMB (%3.0f%%)    ", usedMB, totalMB, pct);
                else if (showVram)
                    text += QString::asprintf("VRAM %.0fMB / %.0fMB    ", usedMB, totalMB);
                else
                    text += QString::asprintf("VRAM %3.0f%%    ", pct);
            } else {
                text += "VRAM N/A    ";
            }
        }
        if (!text.trimmed().isEmpty()) addRow(text.trimmed(), cfg.gpuColor);
    }

    //? ------------------------------------------------ Memory
    if (cfg.showRam) {
        addHeader("Memory", cfg.memColor);
        const auto& mem = Mem::current_mem;
        const double total = static_cast<double>(mem.stats.at("used") + mem.stats.at("available"));
        const double used = static_cast<double>(mem.stats.at("used"));
        const double pct = total > 0 ? used * 100.0 / total : 0.0;

        QString text = "RAM ";
        if (cfg.showRamUsage && cfg.showRamRate)
            text += QString::asprintf("%.0fMB / %.0fMB (%3.0f%%)", used / (1024.0 * 1024.0), total / (1024.0 * 1024.0), pct);
        else if (cfg.showRamUsage)
            text += QString::asprintf("%.0fMB / %.0fMB", used / (1024.0 * 1024.0), total / (1024.0 * 1024.0));
        else if (cfg.showRamRate)
            text += QString::asprintf("%3.0f%%", pct);
        else
            text.clear();
        if (!text.trimmed().isEmpty()) addRow(text.trimmed(), cfg.memColor);

        if (cfg.showSwap && mem.stats.at("swap_total") > 0) {
            const double st = static_cast<double>(mem.stats.at("swap_total"));
            const double su = static_cast<double>(mem.stats.at("swap_used"));
            addRow(QString::asprintf("Swap %.0fMB / %.0fMB (%3.0f%%)", su / (1024.0 * 1024.0), st / (1024.0 * 1024.0), su * 100.0 / st), cfg.memColor);
        }
    }

    //? ------------------------------------------------ Network
    if (cfg.showNet) {
        for (const auto& ifaceStr : cfg.netIfaces) {
            const std::string iface = ifaceStr.toStdString();
            auto it = Net::current_net.find(iface);
            if (it == Net::current_net.end()) continue;
            const auto& ni = it->second;

            QString header = "Network " + ifaceStr;
            if (ni.connected) header += " (up)";
            addHeader(header, cfg.netColor);

            QString row;
            if (const auto& dl = ni.bandwidth.find("download"); dl != ni.bandwidth.end() && !dl->second.empty())
                row += "Down " + humanSpeed(static_cast<double>(dl->second.back()));
            else
                row += "Down 0 B/s";
            row += "   ";
            if (const auto& ul = ni.bandwidth.find("upload"); ul != ni.bandwidth.end() && !ul->second.empty())
                row += "Up " + humanSpeed(static_cast<double>(ul->second.back()));
            else
                row += "Up 0 B/s";
            addRow(row, cfg.netColor);

            if (cfg.netDetailed) {
                QString ip = "IP4 " + QString::fromStdString(ni.ipv4);
                if (!ni.ipv6.empty()) ip += "  IP6 " + QString::fromStdString(ni.ipv6);
                addRow(ip, cfg.netColor);
            }
        }
    }

    //? ------------------------------------------------ Top CPU processes
    if (cfg.showTopProc) {
        const int n = std::max(1, cfg.topProcN);
        std::vector<Proc::proc_info> procs = Proc::current_procs;
        std::stable_sort(procs.begin(), procs.end(),
                         [](const auto& a, const auto& b) { return a.cpu_p > b.cpu_p; });
        if (procs.size() > static_cast<size_t>(n)) procs.resize(n);

        const double totalMem = static_cast<double>(Mem::get_totalMem());
        addHeader(QString("Processes"), cfg.procColor);

        // Table header (column labels aligned with the data rows below).
        QString header = QStringLiteral("Name").leftJustified(20, ' ');
        header += QString("%1").arg("CPU%", 7, QChar(' '));
        header += QString("  %1").arg("MEM%", 6, QChar(' '));
        if (cfg.showProcMemSize) header += QString("  %1").arg("MEM", 9, QChar(' '));
        addRow(header, cfg.procColor);

        for (const auto& p : procs) {
            QString name = QString::fromStdString(p.name);
            if (name.size() > 20) name = name.left(20);
            const double memPct = totalMem > 0 ? p.mem * 100.0 / totalMem : 0.0;
            QString line = name.leftJustified(20, ' ');
            line += QString::asprintf("%6.1f%%", p.cpu_p);
            line += QString::asprintf("  %5.1f%%", memPct);
            if (cfg.showProcMemSize)
                line += QString("  %1").arg(procMemSize(static_cast<double>(p.mem)), 9, QChar(' '));
            addRow(line, cfg.procColor);
        }
    }

    return out;
}
