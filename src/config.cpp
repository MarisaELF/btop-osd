#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

#include "config.h"

void writeSettings(const OsdConfig& cfg){
    QSettings s;

    s.setValue(QStringLiteral("cpu/show"), cfg.showCpu);
    s.setValue(QStringLiteral("cpu/freq"), cfg.showCpuFreq);
    s.setValue(QStringLiteral("cpu/usage"), cfg.showCpuUsage);
    s.setValue(QStringLiteral("cpu/byThread"), cfg.cpuByThread);
    s.setValue(QStringLiteral("cpu/forceCoreLabel"), cfg.forceCoreLabel);
    s.setValue(QStringLiteral("cpu/cols"), cfg.cpuCols);

    for (int i = 0; i < cfg.gpuShow.length(); ++i) {
        const QString k = QStringLiteral("gpu/%1/").arg(i);
        s.setValue(k + QStringLiteral("show"), cfg.gpuShow[i]);
        s.setValue(k + QStringLiteral("vram"), cfg.gpuShowVram[i]);
        s.setValue(k + QStringLiteral("vramRate"), cfg.gpuShowVramRate[i]);
        s.setValue(k + QStringLiteral("usage"), cfg.gpuShowUsage[i]);
    }

    s.setValue(QStringLiteral("ram/show"), cfg.showRam);
    s.setValue(QStringLiteral("ram/usage"), cfg.showRamUsage);
    s.setValue(QStringLiteral("ram/rate"), cfg.showRamRate);
    s.setValue(QStringLiteral("ram/swap"), cfg.showSwap);

    s.setValue(QStringLiteral("net/show"), cfg.showNet);
    s.setValue(QStringLiteral("net/detailed"), cfg.netDetailed);

    s.setValue(QStringLiteral("net/ifaces"), cfg.netIfaces);

    s.setValue(QStringLiteral("proc/show"), cfg.showTopProc);
    s.setValue(QStringLiteral("proc/n"), cfg.topProcN);
    s.setValue(QStringLiteral("proc/memSize"), cfg.showProcMemSize);

    s.setValue(QStringLiteral("osd/position"), cfg.osdPosition);
    s.setValue(QStringLiteral("osd/updateMs"), cfg.updateMs);
    s.setValue(QStringLiteral("osd/marginLR"), cfg.marginLR);
    s.setValue(QStringLiteral("osd/marginTB"), cfg.marginTB);
    s.setValue(QStringLiteral("font/size"), cfg.fontSize);
    s.setValue(QStringLiteral("font/bold"), cfg.fontBold);
    s.setValue(QStringLiteral("osd/mode"), cfg.mode);
    s.setValue(QStringLiteral("osd/bgOpacity"), cfg.bgOpacity);
    s.setValue(QStringLiteral("osd/fontOpacity"), cfg.fontOpacity);
    // Store colors as plain #RRGGBB strings so the config file stays easy to edit by hand.
    s.setValue(QStringLiteral("color/cpu"), cfg.cpuColor.name(QColor::HexRgb));
    s.setValue(QStringLiteral("color/gpu"), cfg.gpuColor.name(QColor::HexRgb));
    s.setValue(QStringLiteral("color/mem"), cfg.memColor.name(QColor::HexRgb));
    s.setValue(QStringLiteral("color/net"), cfg.netColor.name(QColor::HexRgb));
    s.setValue(QStringLiteral("color/proc"), cfg.procColor.name(QColor::HexRgb));
    s.setValue(QStringLiteral("color/background"), cfg.bgColor.name(QColor::HexRgb));

}

OsdConfig readSettings(){
    QSettings s;
    OsdConfig cfg;

    cfg.showCpu = s.value(QStringLiteral("cpu/show"), true).toBool();
    cfg.showCpuFreq = s.value(QStringLiteral("cpu/freq"), true).toBool();
    cfg.showCpuUsage = s.value(QStringLiteral("cpu/usage"), true).toBool();
    cfg.cpuByThread = s.value(QStringLiteral("cpu/byThread"), true).toBool();
    cfg.forceCoreLabel = s.value(QStringLiteral("cpu/forceCoreLabel"), true).toBool();
    cfg.cpuCols = s.value(QStringLiteral("cpu/cols"), 4).toInt();

    // Read GPU settings - determine number of GPUs from settings
    int gpuCount = 0;
    while (s.contains(QStringLiteral("gpu/%1/show").arg(gpuCount))) {
        gpuCount++;
    }
    // If no GPU settings found, default to empty vectors
    cfg.gpuShow.resize(gpuCount);
    cfg.gpuShowVram.resize(gpuCount);
    cfg.gpuShowVramRate.resize(gpuCount);
    cfg.gpuShowUsage.resize(gpuCount);

    for (int i = 0; i < gpuCount; ++i) {
        const QString k = QStringLiteral("gpu/%1/").arg(i);
        cfg.gpuShow[i] = s.value(k + QStringLiteral("show"), false).toBool();
        cfg.gpuShowVram[i] = s.value(k + QStringLiteral("vram"), false).toBool();
        cfg.gpuShowVramRate[i] = s.value(k + QStringLiteral("vramRate"), false).toBool();
        cfg.gpuShowUsage[i] = s.value(k + QStringLiteral("usage"), false).toBool();
    }

    cfg.showRam = s.value(QStringLiteral("ram/show"), true).toBool();
    cfg.showRamUsage = s.value(QStringLiteral("ram/usage"), true).toBool();
    cfg.showRamRate = s.value(QStringLiteral("ram/rate"), true).toBool();
    cfg.showSwap = s.value(QStringLiteral("ram/swap"), false).toBool();

    cfg.showNet = s.value(QStringLiteral("net/show"), true).toBool();
    cfg.netDetailed = s.value(QStringLiteral("net/detailed"), false).toBool();
    cfg.netIfaces = s.value(QStringLiteral("net/ifaces"), QStringList()).toStringList();

    cfg.showTopProc = s.value(QStringLiteral("proc/show"), true).toBool();
    cfg.topProcN = s.value(QStringLiteral("proc/n"), 5).toInt();
    cfg.showProcMemSize = s.value(QStringLiteral("proc/memSize"), true).toBool();

    cfg.osdPosition = s.value(QStringLiteral("osd/position"), 1).toInt();
    cfg.updateMs = s.value(QStringLiteral("osd/updateMs"), 1000).toInt();
    cfg.marginLR = s.value(QStringLiteral("osd/marginLR"), 10).toInt();
    cfg.marginTB = s.value(QStringLiteral("osd/marginTB"), 10).toInt();
    cfg.fontSize = s.value(QStringLiteral("font/size"), 14).toInt();
    cfg.fontBold = s.value(QStringLiteral("font/bold"), true).toBool();
    cfg.mode = s.value(QStringLiteral("osd/mode"), true).toBool();
    cfg.bgOpacity = s.value(QStringLiteral("osd/bgOpacity"), 60).toInt();
    cfg.fontOpacity = s.value(QStringLiteral("osd/fontOpacity"), 100).toInt();
    cfg.cpuColor = QColor(s.value(QStringLiteral("color/cpu"), "#DAA520").toString());
    cfg.gpuColor = QColor(s.value(QStringLiteral("color/gpu"), "#00FF7F").toString());
    cfg.memColor = QColor(s.value(QStringLiteral("color/mem"), "#FF8C00").toString());
    cfg.netColor = QColor(s.value(QStringLiteral("color/net"), "#4FC3F7").toString());
    cfg.procColor = QColor(s.value(QStringLiteral("color/proc"), "#F48FB1").toString());
    cfg.bgColor = QColor(s.value(QStringLiteral("color/background"), "#000000").toString());

    return cfg;
}
