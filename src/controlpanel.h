#pragma once

#include <QCheckBox>
#include <QColor>
#include <QVector>
#include <QWidget>

#include "config.h"

class QComboBox;
class QLabel;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QGroupBox;

//* The btop-osd control panel (menu): CPU / GPU / Memory / Network / Top
//* processes / Appearance (OSD window, font, colors, modes), plus the launch button.
class ControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit ControlPanel(OsdConfig& cfg);

    void config(OsdConfig& cfg) const;
    void setRunning(bool running);
    void loadSettings(const OsdConfig& cfg);
    void saveSettings();

signals:
    void launched(const OsdConfig &cfg);
    void configChanged(const OsdConfig &cfg);
    void quitApp();

private slots:
    void onLaunch();
    void onChanged();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QWidget *buildForm();
    QPushButton *makeColorButton(const QString &label, const QColor &c);
    void updateColorButton(QPushButton *btn, const QColor &c);

    // CPU
    QCheckBox *m_cpuShow = nullptr;
    QCheckBox *m_cpuFreq = nullptr;
    QCheckBox *m_cpuUsage = nullptr;
    QRadioButton *m_cpuThread = nullptr;
    QRadioButton *m_cpuCore = nullptr;
    QCheckBox *m_cpuCoreLabel = nullptr;
    QSpinBox *m_cpuCols = nullptr;

    // GPU (one entry per detected card)
    QVector<QCheckBox*> m_gpuShow;
    QVector<QCheckBox*> m_gpuVram;
    QVector<QCheckBox*> m_gpuVramRate;
    QVector<QCheckBox*> m_gpuUsage;
    QStringList m_gpuNames;

    // Memory
    QCheckBox *m_ramShow = nullptr;
    QCheckBox *m_ramUsage = nullptr;
    QCheckBox *m_ramRate = nullptr;
    QCheckBox *m_ramSwap = nullptr;

    // Network
    QCheckBox *m_netShow = nullptr;
    QCheckBox *m_netDetailed = nullptr;
    QVector<QCheckBox*> m_netIfaces;
    QStringList m_netIfaceNames;

    // Top processes
    QCheckBox *m_procShow = nullptr;
    QSpinBox *m_procN = nullptr;
    QCheckBox *m_procMemSize = nullptr;

    // OSD apparence
    QComboBox *m_position = nullptr;
    QSpinBox *m_updateMs = nullptr;
    QSpinBox *m_fontSize = nullptr;
    QCheckBox *m_fontBold = nullptr;
    QComboBox *m_mode = nullptr;
    QPushButton *m_cpuColorBtn = nullptr;
    QPushButton *m_gpuColorBtn = nullptr;
    QPushButton *m_memColorBtn = nullptr;
    QPushButton *m_netColorBtn = nullptr;
    QPushButton *m_procColorBtn = nullptr;
    QPushButton *m_bgColorBtn = nullptr;
    QSpinBox *m_bgOpacity = nullptr;
    QSpinBox *m_fontOpacity = nullptr;
    QSpinBox *m_marginLR = nullptr;  // left/right padding of the OSD box
    QSpinBox *m_marginTB = nullptr;  // top/bottom padding of the OSD box

    QPushButton *m_launchBtn = nullptr;
    QWidget *m_form = nullptr;
    bool m_loading = false;

    OsdConfig cfg;
};
