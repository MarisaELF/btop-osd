#include "osdwidget.h"

#include <algorithm>

#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QWindow>

#include <LayerShellQt/Window>
#include <sys/socket.h>

namespace {
constexpr int SECTION_GAP = 6;
// Standard 96 DPI point-to-pixel conversion so the OSD text size is
// deterministic and matches typical desktop font sizes on any display.
constexpr qreal PT_TO_PX = 96.0 / 72.0;
}

OsdWidget::OsdWidget(QWidget *parent) : QWidget(parent) {
    // Frameless, tool window, fully click-through.
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowTransparentForInput);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setFixedSize(200, 100);
}

void OsdWidget::setLines(const QVector<OsdLine> &lines, int fontSize, bool forceBold) {
    m_lines = lines;
    m_fontSize = fontSize;
    m_forceBold = forceBold;

    QFont baseFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    baseFont.setPixelSize(qRound(fontSize * PT_TO_PX));
    QFont headerFont = baseFont;
    headerFont.setBold(true);
    headerFont.setPixelSize(qRound((fontSize + 6) * PT_TO_PX));
    setFont(baseFont);

    QFontMetrics rowFm(baseFont);
    QFontMetrics headerFm(headerFont);

    int contentW = 0;
    int contentH = 0;
    bool firstHeader = true;
    for (const auto &line : m_lines) {
        const bool isHeader = line.bold;
        const QFontMetrics &fm = isHeader ? headerFm : rowFm;
        contentW = std::max(contentW, fm.horizontalAdvance(line.text));
        if (isHeader && !firstHeader) contentH += SECTION_GAP;
        contentH += fm.height();
        firstHeader = false;
    }

    const int w = contentW + m_pad * 2;
    const int h = contentH + m_pad * 2;
    setFixedSize(w, h);
    update();
}

//* Distance between the OSD window and the screen edges (layer-shell margins).
void OsdWidget::setMargins(int marginLR, int marginTB) {
    m_marginLR = marginLR;
    m_marginTB = marginTB;
    if (m_layerApplied) applyLayerShell(m_mode);
}

void OsdWidget::setOsdPosition(int position) {
    m_position = position;
    if (m_layerApplied) applyLayerShell(m_mode);
}

void OsdWidget::setOsdMode(bool mode){
    m_mode = mode;
    if (m_layerApplied) applyLayerShell(m_mode);
}

void OsdWidget::setAppearance(const QColor &bgColor, int bgOpacity, int fontOpacity) {
    m_bgColor = bgColor;
    m_bgOpacity = qBound(0, bgOpacity, 100);
    m_fontOpacity = qBound(0, fontOpacity, 100);
    update();
}

void OsdWidget::applyLayerShell(bool mode) {
    if (auto *shellWindow = LayerShellQt::Window::get(windowHandle())) {
        shellWindow->setLayer(mode ? LayerShellQt::Window::LayerOverlay : LayerShellQt::Window::LayerBottom);

        using Anchors = LayerShellQt::Window::Anchors;
        switch (m_position) {
        case 0: shellWindow->setAnchors(static_cast<Anchors>(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorLeft)); break;
        case 2: shellWindow->setAnchors(static_cast<Anchors>(LayerShellQt::Window::AnchorBottom | LayerShellQt::Window::AnchorLeft)); break;
        case 3: shellWindow->setAnchors(static_cast<Anchors>(LayerShellQt::Window::AnchorBottom | LayerShellQt::Window::AnchorRight)); break;
        default: shellWindow->setAnchors(static_cast<Anchors>(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorRight)); break;
        }

        // The margin only applies on the anchored edges (the distance from the screen).
        int left = 0, top = 0, right = 0, bottom = 0;
        switch (m_position) {
        case 0: left = m_marginLR; top = m_marginTB; break;
        case 1: right = m_marginLR; top = m_marginTB; break;
        case 2: left = m_marginLR; bottom = m_marginTB; break;
        default: right = m_marginLR; bottom = m_marginTB; break;
        }
        shellWindow->setMargins(QMargins(left, top, right, bottom));
        shellWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
        shellWindow->setExclusiveZone(0);
        shellWindow->setCloseOnDismissed(true);
    }
}

void OsdWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (!m_layerApplied) {
        m_layerApplied = true;
        applyLayerShell(m_mode);
    }
}

void OsdWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Semi-transparent rounded background.
    QPainterPath bg;
    bg.addRoundedRect(rect(), 8, 8);
    p.fillPath(bg, QColor(m_bgColor.red(), m_bgColor.green(), m_bgColor.blue(),
                          qRound(m_bgOpacity * 2.55)));

    const QFont baseFont = font();
    QFont headerFont = baseFont;
    headerFont.setBold(true);
    headerFont.setPixelSize(qRound((m_fontSize + 6) * PT_TO_PX));

    QFontMetrics rowFm(baseFont);
    QFontMetrics headerFm(headerFont);

    int y = m_pad;
    bool firstHeader = true;
    for (const auto &line : m_lines) {
        const bool isHeader = line.bold;
        const QFontMetrics &fm = isHeader ? headerFm : rowFm;
        if (isHeader && !firstHeader) y += SECTION_GAP;
        y += fm.ascent();
        if (y > height() - fm.descent()) break;

        QFont font = isHeader ? headerFont : baseFont;
        if (m_forceBold) font.setBold(true);

        p.setFont(font);
        const int x = m_pad;
        // Drop shadow.
        p.setPen(QColor(0, 0, 0, 180));
        p.drawText(x + 1, y + 1, line.text);
        // Foreground.
        QColor fg = line.color;
        fg.setAlpha(qRound(m_fontOpacity * 2.55));
        p.setPen(fg);
        p.drawText(x, y, line.text);

        y += fm.descent() + fm.leading();
        firstHeader = false;
    }
}
