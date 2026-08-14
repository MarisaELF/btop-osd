#pragma once
#include <QVector>
#include <QColor>

struct OsdConfig {
    // CPU
    bool showCpu = true;
    bool showCpuFreq = true;
    bool showCpuUsage = true;
    bool cpuByThread = true;     // true = one entry per logical thread, false = per physical core
    bool forceCoreLabel = true;  // always print "Core" even when showing threads
    int cpuCols = 4;

    // GPU (per detected card)
    QVector<bool> gpuShow;
    QVector<bool> gpuShowVram;
    QVector<bool> gpuShowVramRate;
    QVector<bool> gpuShowUsage;

    // Memory
    bool showRam = true;
    bool showRamUsage = true;
    bool showRamRate = true;
    bool showSwap = false;

    // Apparence
    int osdPosition = 1; // 0=top-left 1=top-right 2=bottom-left 3=bottom-right
    int updateMs = 1000;
    int fontSize = 8;
    bool fontBold = true;
    int marginLR = 10;
    int marginTB = 10;
    bool mode = true;  // true = overlay layer on top, false = bottom layer
    QColor cpuColor = QColor("#DAA520");
    QColor gpuColor = QColor("#00FF7F");
    QColor memColor = QColor("#FF8C00");
    QColor netColor = QColor("#4FC3F7");
    QColor procColor = QColor("#F48FB1");
    QColor bgColor = QColor("#000000");
    int bgOpacity = 60;      // background opacity in percent (0-100)
    int fontOpacity = 100;   // text opacity in percent (0-100)

    // Network
    bool showNet = true;
    bool netDetailed = false;
    QStringList netIfaces;

    // Top processes
    bool showTopProc = true;
    int topProcN = 5;
    bool showProcMemSize = true;  // show the resident memory size column in the top process list
};

void writeSettings(const OsdConfig& cfg);
OsdConfig readSettings();
