#pragma once

#include <QColor>
#include <QString>
#include <QStringList>
#include <QVector>

#include "config.h"

//? A single line of OSD text (a header or a data row).
struct OsdLine {
    QString text;
    QColor color;
    bool bold = false;
};

namespace OsdEngine {
    //* One-time initialization of the btop collectors.
    void init();

    //* Refresh all collectors that are needed by the current config.
    void collect(const OsdConfig &cfg);

    //* Build the list of lines to render for the given config.
    QVector<OsdLine> buildLines(const OsdConfig &cfg);

    //* Available network interfaces (populated after init()).
    QStringList interfaces();

    //* Names of the detected GPUs.
    QStringList gpuNames();
};
