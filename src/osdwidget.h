#pragma once

#include <QWidget>

#include "osdengine.h"

//* Wayland layer-shell OSD overlay: transparent, click-through, renders a list
//* of colored monospace lines on top of everything else.
class OsdWidget : public QWidget {
    Q_OBJECT
public:
    explicit OsdWidget(QWidget *parent = nullptr);

    void setLines(const QVector<OsdLine> &lines, int fontSize, bool forceBold);
    void setAppearance(const QColor &bgColor, int bgOpacity, int fontOpacity);
    void setMargins(int marginLR, int marginTB);
    void setOsdPosition(int position); // 0=TL 1=TR 2=BL 3=BR
    void setOsdMode(bool mode);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void applyLayerShell(bool mode);

    QVector<OsdLine> m_lines;
    int m_fontSize = 14;
    bool m_forceBold = true;
    int m_position = 1;
    int m_pad = 10;
    bool m_layerApplied = false;
    bool m_mode = true;
    QColor m_bgColor = QColor(Qt::black);
    int m_bgOpacity = 60;
    int m_fontOpacity = 100;
    int m_marginLR = 10;  // OSD window distance from the left/right screen edge
    int m_marginTB = 10;  // OSD window distance from the top/bottom screen edge
};
