#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QTextStream>
#include <qtmetamacros.h>

#include "config.h"
#include "controlpanel.h"
#include "osdengine.h"
#include "osdwidget.h"

int main(int argc, char *argv[]) {
    QApplication::setApplicationName(QStringLiteral("btop-osd"));
    QApplication::setOrganizationName(QStringLiteral("btop-osd"));
    QApplication app(argc, argv);

    OsdEngine::init();

    OsdConfig cfg = readSettings();

    OsdWidget osd;
    ControlPanel panel(cfg);

    QTimer updateTimer;
    bool running = false;

    auto renderOnce = [&]() {
        OsdEngine::collect(cfg);
        osd.setAppearance(cfg.bgColor, cfg.bgOpacity, cfg.fontOpacity);
        osd.setLines(OsdEngine::buildLines(cfg), cfg.fontSize, cfg.fontBold);
    };

    // Static preview: render one frame with the current config and show it,
    // no timer refresh. Re-run whenever the panel form changes.
    auto showPreview = [&]() {
        osd.setOsdPosition(cfg.osdPosition);
        osd.setOsdMode(cfg.mode);
        osd.setMargins(cfg.marginLR, cfg.marginTB);
        renderOnce();
        osd.show();
    };

    auto startOsd = [&](const OsdConfig& menuCfg) {
        cfg = menuCfg;
        osd.setOsdPosition(cfg.osdPosition);
        osd.setOsdMode(cfg.mode);
        osd.setMargins(cfg.marginLR, cfg.marginTB);
        renderOnce();
        updateTimer.setInterval(cfg.updateMs);
        updateTimer.start();
        running = true;
        osd.show();
        panel.hide();
    };

    auto stopOsd = [&]() {
        if (!running) return;
        running = false;
        updateTimer.stop();
        panel.setRunning(false);
    };


    QObject::connect(&panel, &ControlPanel::launched, &panel, startOsd);

    QObject::connect(&panel, &ControlPanel::configChanged, &panel, [&](const OsdConfig& c) {
        cfg = c;
        showPreview();
    });

    QObject::connect(&updateTimer, &QTimer::timeout, &updateTimer, [&]() {
        if (running) renderOnce();
    });

    //? System tray
    auto *trayMenu = new QMenu();
    QAction *openAction = trayMenu->addAction(QStringLiteral("Open Settings"));
    trayMenu->addSeparator();
    QAction *quitAction = trayMenu->addAction(QStringLiteral("Quit"));

    QSystemTrayIcon tray(QIcon(QStringLiteral(":/icon.png")));
    tray.setToolTip(QStringLiteral("btop-osd"));
    tray.setContextMenu(trayMenu);
    tray.show();

    QObject::connect(openAction, &QAction::triggered, &app, [&]() {
        stopOsd();
        showPreview();
        panel.show();
        panel.raise();
        panel.activateWindow();
    });

    QObject::connect(&tray, &QSystemTrayIcon::activated, &app, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            stopOsd();
            showPreview();
            panel.show();
            panel.raise();
            panel.activateWindow();
        }
    });

    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    QObject::connect(&panel, &ControlPanel::quitApp, &app, &QApplication::quit);

    const QString arg = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : QString();
    if (arg == QLatin1String("autorun") || arg == QLatin1String("--autorun")) {
        startOsd(cfg);
    } else {
        panel.resize(720, 620);
        panel.show();
        showPreview();
    }

    return app.exec();
}
