#include "controlpanel.h"
#include "config.h"
#include "osdengine.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
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
#include <QVariant>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <qcontainerfwd.h>

ControlPanel::ControlPanel(OsdConfig& cfg) : cfg(cfg) {
    setWindowTitle(QStringLiteral("btop-osd Control Panel"));
    setWindowIcon(QIcon(":/icon.png"));

    m_gpuNames = OsdEngine::gpuNames();
    m_netIfaceNames = OsdEngine::interfaces();

    auto *root = new QVBoxLayout(this);

    auto *title = new QLabel(QStringLiteral("btop-osd Control Panel"), this);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    root->addWidget(title);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_form = buildForm();
    scroll->setWidget(m_form);
    root->addWidget(scroll, 1);

    m_launchBtn = new QPushButton(QStringLiteral("Launch"), this);
    m_launchBtn->setMinimumHeight(34);
    connect(m_launchBtn, &QPushButton::clicked, this, &ControlPanel::onLaunch);
    root->addWidget(m_launchBtn);

    loadSettings(cfg);
}

QWidget *ControlPanel::buildForm() {
    auto *form = new QWidget(this);
    auto *layout = new QVBoxLayout(form);

    //? ------------------------------------------- CPU
    auto *cpuBox = new QGroupBox(QStringLiteral("CPU Settings"), form);
    auto *cpuLay = new QVBoxLayout(cpuBox);
    m_cpuShow = new QCheckBox(QStringLiteral("Show CPU Information"), cpuBox);
    m_cpuShow->setChecked(true);
    m_cpuFreq = new QCheckBox(QStringLiteral("Show Frequency"), cpuBox);
    m_cpuFreq->setChecked(true);
    m_cpuUsage = new QCheckBox(QStringLiteral("Show Usage"), cpuBox);
    m_cpuUsage->setChecked(true);
    auto *cpuDisplay = new QHBoxLayout();
    cpuDisplay->addWidget(m_cpuFreq);
    cpuDisplay->addWidget(m_cpuUsage);

    m_cpuThread = new QRadioButton(QStringLiteral("By Thread"), cpuBox);
    m_cpuCore = new QRadioButton(QStringLiteral("By Core"), cpuBox);
    m_cpuThread->setChecked(true);
    auto *cpuGroup = new QButtonGroup(cpuBox);
    cpuGroup->addButton(m_cpuThread);
    cpuGroup->addButton(m_cpuCore);
    m_cpuCoreLabel = new QCheckBox(QStringLiteral("Force 'Core' Label"), cpuBox);
    m_cpuCoreLabel->setChecked(true);
    auto *cpuUnit = new QHBoxLayout();
    cpuUnit->addWidget(m_cpuCore);
    cpuUnit->addWidget(m_cpuThread);
    cpuUnit->addWidget(m_cpuCoreLabel);

    m_cpuCols = new QSpinBox(cpuBox);
    m_cpuCols->setRange(1, 16);
    m_cpuCols->setValue(4);
    auto *cpuColsLay = new QHBoxLayout();
    cpuColsLay->addWidget(new QLabel(QStringLiteral("Columns:"), cpuBox));
    cpuColsLay->addWidget(m_cpuCols);
    cpuColsLay->addStretch(1);

    auto *cpuSub = new QVBoxLayout();
    cpuSub->addLayout(cpuDisplay);
    cpuSub->addLayout(cpuUnit);
    cpuSub->addLayout(cpuColsLay);
    cpuLay->addWidget(m_cpuShow);
    cpuLay->addLayout(cpuSub);
    cpuLay->addStretch(1);
    layout->addWidget(cpuBox);

    //? ------------------------------------------- GPU (one per card)
    for (int i = 0; i < m_gpuNames.size(); ++i) {
        QString name = m_gpuNames[i].isEmpty() ? QStringLiteral("GPU %1").arg(i) : m_gpuNames[i];
        auto *gpuBox = new QGroupBox(QStringLiteral("GPU #%1 (%2)").arg(i).arg(name), form);
        auto *gpuLay = new QVBoxLayout(gpuBox);

        auto *showGpu = new QCheckBox(QStringLiteral("Show this GPU"), gpuBox);
        showGpu->setChecked(true);
        auto *showVram = new QCheckBox(QStringLiteral("VRAM size"), gpuBox);
        showVram->setChecked(true);
        auto *showVramRate = new QCheckBox(QStringLiteral("VRAM %"), gpuBox);
        showVramRate->setChecked(true);
        auto *showUsage = new QCheckBox(QStringLiteral("GPU %"), gpuBox);
        showUsage->setChecked(true);

        m_gpuShow.append(showGpu);
        m_gpuVram.append(showVram);
        m_gpuVramRate.append(showVramRate);
        m_gpuUsage.append(showUsage);

        auto *row = new QHBoxLayout();
        row->addWidget(showVram);
        row->addWidget(showVramRate);
        row->addWidget(showUsage);
        gpuLay->addWidget(showGpu);
        gpuLay->addLayout(row);
        gpuLay->addStretch(1);
        layout->addWidget(gpuBox);
    }

    //? ------------------------------------------- Memory
    auto *ramBox = new QGroupBox(QStringLiteral("Memory Settings"), form);
    auto *ramLay = new QVBoxLayout(ramBox);
    m_ramShow = new QCheckBox(QStringLiteral("Show Memory Information"), ramBox);
    m_ramShow->setChecked(true);
    m_ramUsage = new QCheckBox(QStringLiteral("RAM Usage (size)"), ramBox);
    m_ramUsage->setChecked(true);
    m_ramRate = new QCheckBox(QStringLiteral("RAM %"), ramBox);
    m_ramRate->setChecked(true);
    m_ramSwap = new QCheckBox(QStringLiteral("Show Swap"), ramBox);
    m_ramSwap->setChecked(false);
    auto *ramRow = new QHBoxLayout();
    ramRow->addWidget(m_ramUsage);
    ramRow->addWidget(m_ramRate);
    ramRow->addWidget(m_ramSwap);
    ramLay->addWidget(m_ramShow);
    ramLay->addLayout(ramRow);
    ramLay->addStretch(1);
    layout->addWidget(ramBox);

    //? ------------------------------------------- Network
    auto *netBox = new QGroupBox(QStringLiteral("Network Settings"), form);
    auto *netLay = new QVBoxLayout(netBox);
    m_netShow = new QCheckBox(QStringLiteral("Show Network"), netBox);
    m_netShow->setChecked(true);
    m_netDetailed = new QCheckBox(QStringLiteral("Show IP addresses"), netBox);
    m_netDetailed->setChecked(false);
    netLay->addWidget(m_netShow);
    netLay->addWidget(m_netDetailed);
    netLay->addWidget(new QLabel(QStringLiteral("Interfaces:"), netBox));
    for (const auto& name : m_netIfaceNames) {
        auto *cb = new QCheckBox(name, netBox);
        m_netIfaces.append(cb);
        netLay->addWidget(cb);
    }
    netLay->addStretch(1);
    layout->addWidget(netBox);

    //? ------------------------------------------- Top processes
    auto *procBox = new QGroupBox(QStringLiteral("Processes"), form);
    auto *procLay = new QHBoxLayout(procBox);
    m_procShow = new QCheckBox(QStringLiteral("Show Processes"), procBox);
    m_procShow->setChecked(true);
    m_procN = new QSpinBox(procBox);
    m_procN->setRange(1, 50);
    m_procN->setValue(5);
    m_procMemSize = new QCheckBox(QStringLiteral("Memory size"), procBox);
    m_procMemSize->setChecked(true);
    procLay->addWidget(m_procShow);
    procLay->addWidget(new QLabel(QStringLiteral("Count:"), procBox));
    procLay->addWidget(m_procN);
    procLay->addWidget(m_procMemSize);
    procLay->addStretch(1);
    layout->addWidget(procBox);

    //? ------------------------------------------- Appearance (OSD window + font + colors)
    auto *appBox = new QGroupBox(QStringLiteral("Appearance"), form);
    auto *appLay = new QFormLayout(appBox);

    m_position = new QComboBox(appBox);
    m_position->addItem(QStringLiteral("Top-Left"));
    m_position->addItem(QStringLiteral("Top-Right"));
    m_position->addItem(QStringLiteral("Bottom-Left"));
    m_position->addItem(QStringLiteral("Bottom-Right"));
    m_position->setCurrentIndex(1);

    m_updateMs = new QSpinBox(appBox);
    m_updateMs->setRange(200, 10000);
    m_updateMs->setSingleStep(100);
    m_updateMs->setValue(1000);

    m_mode = new QComboBox(appBox);
    m_mode->addItem(QStringLiteral("Overlay (on top)"));
    m_mode->addItem(QStringLiteral("Bottom layer"));
    m_mode->setCurrentIndex(cfg.mode ? 0 : 1);

    m_fontSize = new QSpinBox(appBox);
    m_fontSize->setRange(6, 40);
    m_fontSize->setValue(14);
    m_fontBold = new QCheckBox(QStringLiteral("Bold (RTSS style)"), appBox);
    m_fontBold->setChecked(true);
    auto *fontRow = new QWidget(appBox);
    auto *fontRowLay = new QHBoxLayout(fontRow);
    fontRowLay->setContentsMargins(0, 0, 0, 0);
    fontRowLay->addWidget(m_fontSize);
    fontRowLay->addWidget(m_fontBold);
    fontRowLay->addStretch(1);

    m_fontOpacity = new QSpinBox(appBox);
    m_fontOpacity->setRange(0, 100);
    m_fontOpacity->setSuffix(QStringLiteral(" %"));
    m_fontOpacity->setValue(100);

    m_cpuColorBtn = makeColorButton(QStringLiteral("CPU"), cfg.cpuColor);
    m_gpuColorBtn = makeColorButton(QStringLiteral("GPU"), cfg.gpuColor);
    m_memColorBtn = makeColorButton(QStringLiteral("Memory"), cfg.memColor);
    m_netColorBtn = makeColorButton(QStringLiteral("Network"), cfg.netColor);
    m_procColorBtn = makeColorButton(QStringLiteral("Processes"), cfg.procColor);
    auto *colorRow = new QWidget(appBox);
    auto *colorLay = new QHBoxLayout(colorRow);
    colorLay->setContentsMargins(0, 0, 0, 0);
    colorLay->addWidget(m_cpuColorBtn);
    colorLay->addWidget(m_gpuColorBtn);
    colorLay->addWidget(m_memColorBtn);
    colorLay->addWidget(m_netColorBtn);
    colorLay->addWidget(m_procColorBtn);

    m_bgColorBtn = makeColorButton(QStringLiteral("Background"), cfg.bgColor);
    m_bgOpacity = new QSpinBox(appBox);
    m_bgOpacity->setRange(0, 100);
    m_bgOpacity->setSuffix(QStringLiteral(" %"));
    m_bgOpacity->setValue(60);

    m_marginLR = new QSpinBox(appBox);
    m_marginLR->setRange(0, 100);
    m_marginLR->setSuffix(QStringLiteral(" px"));
    m_marginLR->setValue(10);
    m_marginTB = new QSpinBox(appBox);
    m_marginTB->setRange(0, 100);
    m_marginTB->setSuffix(QStringLiteral(" px"));
    m_marginTB->setValue(10);

    appLay->addRow(QStringLiteral("Position:"), m_position);
    appLay->addRow(QStringLiteral("Update (ms):"), m_updateMs);
    appLay->addRow(QStringLiteral("Mode:"), m_mode);
    appLay->addRow(QStringLiteral("Font size:"), fontRow);
    appLay->addRow(QStringLiteral("Font opacity:"), m_fontOpacity);
    appLay->addRow(QStringLiteral("Margin L/R:"), m_marginLR);
    appLay->addRow(QStringLiteral("Margin T/B:"), m_marginTB);
    appLay->addRow(QStringLiteral("Section colors:"), colorRow);
    appLay->addRow(QStringLiteral("Background color:"), m_bgColorBtn);
    appLay->addRow(QStringLiteral("Background opacity:"), m_bgOpacity);
    layout->addWidget(appBox);

    //? Any change in the form refreshes the OSD preview once.
    const auto buttons = form->findChildren<QAbstractButton *>();
    for (auto *b : buttons) {
        if (b->property("noPreview").toBool()) continue;
        connect(b, &QAbstractButton::clicked, this, &ControlPanel::onChanged);
    }
    const auto combos = form->findChildren<QComboBox *>();
    for (auto *c : combos)
        connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ControlPanel::onChanged);
    const auto spins = form->findChildren<QSpinBox *>();
    for (auto *s : spins)
        connect(s, QOverload<int>::of(&QSpinBox::valueChanged), this, &ControlPanel::onChanged);

    layout->addStretch(1);
    return form;
}

void ControlPanel::config(OsdConfig& cfg) const {
    cfg.showCpu = m_cpuShow->isChecked();
    cfg.showCpuFreq = m_cpuFreq->isChecked();
    cfg.showCpuUsage = m_cpuUsage->isChecked();
    cfg.cpuByThread = m_cpuThread->isChecked();
    cfg.forceCoreLabel = m_cpuCoreLabel->isChecked();
    cfg.cpuCols = m_cpuCols->value();

    const int gpuCount = m_gpuShow.size();
    cfg.gpuShow.resize(gpuCount);
    cfg.gpuShowVram.resize(gpuCount);
    cfg.gpuShowVramRate.resize(gpuCount);
    cfg.gpuShowUsage.resize(gpuCount);
    for (int i = 0; i < gpuCount; ++i) {
        cfg.gpuShow[i] = m_gpuShow[i]->isChecked();
        cfg.gpuShowVram[i] = m_gpuVram[i]->isChecked();
        cfg.gpuShowVramRate[i] = m_gpuVramRate[i]->isChecked();
        cfg.gpuShowUsage[i] = m_gpuUsage[i]->isChecked();
    }

    cfg.showRam = m_ramShow->isChecked();
    cfg.showRamUsage = m_ramUsage->isChecked();
    cfg.showRamRate = m_ramRate->isChecked();
    cfg.showSwap = m_ramSwap->isChecked();

    cfg.showNet = m_netShow->isChecked();
    cfg.netDetailed = m_netDetailed->isChecked();
    cfg.netIfaces = QStringList();
    for (int i = 0; i < m_netIfaces.size() && i < m_netIfaceNames.size(); ++i) {
        if (m_netIfaces[i]->isChecked()) cfg.netIfaces << m_netIfaceNames[i];
    }

    cfg.showTopProc = m_procShow->isChecked();
    cfg.topProcN = m_procN->value();
    cfg.showProcMemSize = m_procMemSize->isChecked();

    cfg.osdPosition = m_position->currentIndex();
    cfg.updateMs = m_updateMs->value();
    cfg.mode = m_mode->currentIndex() == 0;
    cfg.fontSize = m_fontSize->value();
    cfg.fontBold = m_fontBold->isChecked();
    cfg.fontOpacity = m_fontOpacity->value();
    cfg.bgOpacity = m_bgOpacity->value();
    cfg.marginLR = m_marginLR->value();
    cfg.marginTB = m_marginTB->value();
    cfg.cpuColor = m_cpuColorBtn->property("qcolor").value<QColor>();
    cfg.gpuColor = m_gpuColorBtn->property("qcolor").value<QColor>();
    cfg.memColor = m_memColorBtn->property("qcolor").value<QColor>();
    cfg.netColor = m_netColorBtn->property("qcolor").value<QColor>();
    cfg.procColor = m_procColorBtn->property("qcolor").value<QColor>();
    cfg.bgColor = m_bgColorBtn->property("qcolor").value<QColor>();
}

void ControlPanel::onChanged() {
    if (m_loading) return;
    config(cfg);
    emit configChanged(cfg);
}

void ControlPanel::setRunning(bool running) {
    m_form->setEnabled(!running);
}

void ControlPanel::onLaunch() {
    config(cfg);
    writeSettings(cfg);
    emit launched(cfg);
    setRunning(true);
}

void ControlPanel::loadSettings(const OsdConfig &cfg) {
    m_loading = true;

    // CPU
    m_cpuShow->setChecked(cfg.showCpu);
    m_cpuFreq->setChecked(cfg.showCpuFreq);
    m_cpuUsage->setChecked(cfg.showCpuUsage);
    m_cpuThread->setChecked(cfg.cpuByThread);
    m_cpuCore->setChecked(!cfg.cpuByThread);
    m_cpuCoreLabel->setChecked(cfg.forceCoreLabel);
    m_cpuCols->setValue(cfg.cpuCols);

    // GPU
    const int gpuCount = cfg.gpuShow.size();
    for (int i = 0; i < gpuCount; ++i) {
        m_gpuShow[i]->setChecked(cfg.gpuShow[i]);
        m_gpuVram[i]->setChecked(cfg.gpuShowVram[i]);
        m_gpuVramRate[i]->setChecked(cfg.gpuShowVramRate[i]);
        m_gpuUsage[i]->setChecked(cfg.gpuShowUsage[i]);
    }

    // Memory
    m_ramShow->setChecked(cfg.showRam);
    m_ramUsage->setChecked(cfg.showRamUsage);
    m_ramRate->setChecked(cfg.showRamRate);
    m_ramSwap->setChecked(cfg.showSwap);

    // Network
    m_netShow->setChecked(cfg.showNet);
    m_netDetailed->setChecked(cfg.netDetailed);
    if (!cfg.netIfaces.isEmpty()) {
        for (int i = 0; i < m_netIfaces.size(); ++i) {
            m_netIfaces[i]->setChecked(cfg.netIfaces.contains(m_netIfaceNames[i]));
        }
    } else {
        for (int i = 0; i < m_netIfaces.size(); ++i) {
            m_netIfaces[i]->setChecked(true);
        }
    }

    // Top processes
    m_procShow->setChecked(cfg.showTopProc);
    m_procN->setValue(cfg.topProcN);
    m_procMemSize->setChecked(cfg.showProcMemSize);

    // OSD appearance
    m_position->setCurrentIndex(cfg.osdPosition);
    m_updateMs->setValue(cfg.updateMs);
    m_mode->setCurrentIndex(cfg.mode ? 0 : 1);
    m_fontSize->setValue(cfg.fontSize);
    m_fontBold->setChecked(cfg.fontBold);
    m_fontOpacity->setValue(cfg.fontOpacity);
    m_bgOpacity->setValue(cfg.bgOpacity);
    m_marginLR->setValue(cfg.marginLR);
    m_marginTB->setValue(cfg.marginTB);
    m_cpuColorBtn->setProperty("qcolor", QVariant::fromValue(cfg.cpuColor));
    updateColorButton(m_cpuColorBtn, cfg.cpuColor);
    m_gpuColorBtn->setProperty("qcolor", QVariant::fromValue(cfg.gpuColor));
    updateColorButton(m_gpuColorBtn, cfg.gpuColor);
    m_memColorBtn->setProperty("qcolor", QVariant::fromValue(cfg.memColor));
    updateColorButton(m_memColorBtn, cfg.memColor);
    m_netColorBtn->setProperty("qcolor", QVariant::fromValue(cfg.netColor));
    updateColorButton(m_netColorBtn, cfg.netColor);
    m_procColorBtn->setProperty("qcolor", QVariant::fromValue(cfg.procColor));
    updateColorButton(m_procColorBtn, cfg.procColor);
    m_bgColorBtn->setProperty("qcolor", QVariant::fromValue(cfg.bgColor));
    updateColorButton(m_bgColorBtn, cfg.bgColor);

    m_loading = false;
}

QPushButton *ControlPanel::makeColorButton(const QString &label, const QColor &c) {
    auto *btn = new QPushButton(this);
    btn->setProperty("noPreview", true);
    btn->setProperty("colorLabel", label);
    btn->setProperty("qcolor", QVariant::fromValue(c));
    updateColorButton(btn, c);
    connect(btn, &QPushButton::clicked, this, [this, btn]() {
        const QColor cur = btn->property("qcolor").value<QColor>();
        const QColor picked = QColorDialog::getColor(cur, this, QStringLiteral("Pick color"));
        if (!picked.isValid()) return;
        btn->setProperty("qcolor", QVariant::fromValue(picked));
        updateColorButton(btn, picked);
        onChanged();
    });
    return btn;
}

void ControlPanel::updateColorButton(QPushButton *btn, const QColor &c) {
    const QColor textColor = c.lightness() > 128 ? QColor(Qt::black) : QColor(Qt::white);
    btn->setText(QStringLiteral("%1 %2").arg(btn->property("colorLabel").toString(), c.name(QColor::HexRgb)));
    btn->setStyleSheet(QStringLiteral("QPushButton { background-color: %1; color: %2; border: 1px solid #555; }")
                           .arg(c.name(QColor::HexRgb), textColor.name()));
}

void ControlPanel::closeEvent(QCloseEvent *event)
{
    if (event->spontaneous()) {
        writeSettings(cfg);
        emit quitApp();
    } else {
        QWidget::closeEvent(event);
    }
}
