#include "instrument_panel.h"
#include "instrument_detector.h"
#include "led_matrix_widget.h"
#include <QHBoxLayout>
#include <QFrame>

InstrumentPanel::InstrumentPanel(QWidget *parent)
    : QDockWidget("Instruments", parent)
{
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea | Qt::BottomDockWidgetArea);
    setMinimumWidth(240);

    m_container = new QWidget;
    m_layout = new QVBoxLayout(m_container);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);

    // Header
    m_header = new QLabel("Instruments");
    m_header->setStyleSheet("font-weight: bold; padding: 4px; color: #cbd5e1; font-size: 12px;");
    m_layout->addWidget(m_header);

    // Empty state
    m_emptyLabel = new QLabel("Run a simulation to detect instruments");
    m_emptyLabel->setStyleSheet("color: #64748b; padding: 16px; font-style: italic;");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_emptyLabel);

    // Scroll area for instrument widgets
    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { width: 8px; background: #1a1a2e; }"
        "QScrollBar::handle:vertical { background: #3a3a5e; border-radius: 4px; min-height: 20px; }");
    auto *scrollContent = new QWidget;
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(8);
    // Reserve layout for instrument widgets (added by addLedMatrix)
    scrollContent->setLayout(scrollLayout);
    m_scroll->setWidget(scrollContent);

    // Time controls (hidden by default)
    m_timeWidget = new QWidget;
    auto *timeLayout = new QVBoxLayout(m_timeWidget);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->setSpacing(2);

    auto *controlsRow = new QHBoxLayout;
    m_playBtn = new QPushButton("▶");
    m_playBtn->setFixedSize(28, 24);
    m_playBtn->setToolTip("Play/Pause animation");
    m_playBtn->setStyleSheet(
        "QPushButton { background: #2a2a4e; color: #00f2fe; border: 1px solid #3a3a5e;"
        "  border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background: #3a3a5e; }"
        "QPushButton:checked { background: #1a4a3e; color: #ff6b6b; }");
    m_playBtn->setCheckable(true);

    m_speedCombo = new QComboBox;
    m_speedCombo->addItem("1×", 1.0);
    m_speedCombo->addItem("10×", 10.0);
    m_speedCombo->addItem("100×", 100.0);
    m_speedCombo->addItem("1000×", 1000.0);
    m_speedCombo->addItem("10000×", 10000.0);
    m_speedCombo->setCurrentIndex(2);
    m_speedCombo->setStyleSheet(
        "QComboBox { background: #2a2a4e; color: #cbd5e1; border: 1px solid #3a3a5e;"
        "  border-radius: 4px; padding: 2px 6px; font-size: 10px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: #1a1a2e; color: #cbd5e1;"
        "  selection-background-color: #3a3a5e; }");

    controlsRow->addWidget(m_playBtn);
    controlsRow->addWidget(m_speedCombo, 1);
    timeLayout->addLayout(controlsRow);

    m_timeSlider = new QSlider(Qt::Horizontal);
    m_timeSlider->setRange(0, 10000);
    m_timeSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 4px; background: #2a2a4e; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #00f2fe; width: 12px; height: 12px;"
        "  margin: -4px 0; border-radius: 6px; }"
        "QSlider::sub-page:horizontal { background: #4facfe; border-radius: 2px; }");
    timeLayout->addWidget(m_timeSlider);

    m_timeLabel = new QLabel("t = 0 s");
    m_timeLabel->setStyleSheet("color: #94a3b8; font-size: 10px; font-family: monospace;");
    timeLayout->addWidget(m_timeLabel);

    m_timeWidget->setLayout(timeLayout);
    m_timeWidget->hide();

    m_layout->addWidget(m_scroll, 1);
    m_layout->addWidget(m_timeWidget);

    setWidget(m_container);

    // Connections
    connect(m_timeSlider, &QSlider::sliderPressed, this, [this]() {
        m_sliderDragging = true;
    });
    connect(m_timeSlider, &QSlider::sliderReleased, this, [this]() {
        m_sliderDragging = false;
        onSliderChanged(m_timeSlider->value());
    });
    connect(m_timeSlider, &QSlider::valueChanged, this, [this](int) {
        if (m_sliderDragging)
            onSliderChanged(m_timeSlider->value());
    });
    connect(m_playBtn, &QPushButton::toggled, this, &InstrumentPanel::onPlayPause);
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &InstrumentPanel::onSpeedChanged);
}

void InstrumentPanel::clearInstruments()
{
    auto *scrollContent = m_scroll->widget();
    if (!scrollContent) return;
    auto *layout = scrollContent->layout();
    if (!layout) return;

    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }
    m_instruments.clear();
    m_hasData = false;
    m_timeWidget->hide();
    m_emptyLabel->show();
}

void InstrumentPanel::addLedMatrix(const InstrumentDef &def,
                                    const QMap<QString, SignalRef> &signalData)
{
    m_emptyLabel->hide();

    auto *scrollContent = m_scroll->widget();
    auto *layout = scrollContent->layout();

    auto *matrix = new LedMatrixWidget(def.rows, def.cols);
    matrix->setSignalNames(def.signalNames);
    matrix->setSignalData(signalData);

    layout->addWidget(matrix);
    m_instruments[def.name] = matrix;

    connect(matrix, &LedMatrixWidget::timeChanged,
            this, &InstrumentPanel::onInstrumentTimeChanged);

    m_hasData = true;
    rebuildTimeControls();
}

void InstrumentPanel::setSignalData(const QMap<QString, SignalRef> &data)
{
    for (auto it = m_instruments.begin(); it != m_instruments.end(); ++it) {
        it.value()->setSignalData(data);
    }
}

void InstrumentPanel::setProbeTime(double seconds)
{
    m_probeTime = seconds;
    for (auto it = m_instruments.begin(); it != m_instruments.end(); ++it) {
        it.value()->setTime(seconds);
    }

    if (!m_sliderDragging && m_timeMax > m_timeMin) {
        double frac = (seconds - m_timeMin) / (m_timeMax - m_timeMin);
        m_timeSlider->blockSignals(true);
        m_timeSlider->setValue((int)(frac * 10000));
        m_timeSlider->blockSignals(false);
    }

    m_timeLabel->setText(QString("t = %1 s").arg(seconds, 0, 'e', 3));
}

void InstrumentPanel::onSliderChanged(int value)
{
    if (m_timeMax <= m_timeMin) return;
    double frac = value / 10000.0;
    double t = m_timeMin + frac * (m_timeMax - m_timeMin);
    setProbeTime(t);
    emit probeTimeChanged(t);
}

void InstrumentPanel::onPlayPause()
{
    if (m_playBtn->isChecked()) {
        double from = m_probeTime;
        double to = m_timeMax;
        for (auto it = m_instruments.begin(); it != m_instruments.end(); ++it) {
            it.value()->play(from, to);
        }
    } else {
        for (auto it = m_instruments.begin(); it != m_instruments.end(); ++it) {
            it.value()->pause();
        }
    }
}

void InstrumentPanel::onSpeedChanged(int index)
{
    double speed = m_speedCombo->itemData(index).toDouble();
    for (auto it = m_instruments.begin(); it != m_instruments.end(); ++it) {
        it.value()->setAnimationSpeed(speed);
    }
}

void InstrumentPanel::onInstrumentTimeChanged(double t)
{
    m_probeTime = t;
    // Sync other instruments
    for (auto it = m_instruments.begin(); it != m_instruments.end(); ++it) {
        if (it.value()->time() != t)
            it.value()->setTime(t);
    }
    emit probeTimeChanged(t);
}

void InstrumentPanel::rebuildTimeControls()
{
    // Compute time range from all instrument signal data
    m_timeMin = 0.0;
    m_timeMax = 1.0;

    // Walk all signal refs across all instruments to find global range
    bool found = false;
    for (auto it = m_instruments.begin(); it != m_instruments.end(); ++it) {
        auto *matrix = it.value();
        for (const auto &sigName : matrix->signalNames()) {
            (void)sigName;
        }
    }

    // For now use a heuristic — set range from first LED widget's data
    if (!m_instruments.isEmpty()) {
        // Query the first instrument's signal range
        for (auto it = m_instruments.begin(); it != m_instruments.end(); ++it) {
            auto *w = it.value();
            if (w->signalNames().isEmpty()) continue;
            // Get the signal ref for the first signal
            // We don't have direct access to the refs through the widget,
            // so we skip range detection for now. The slider will be 0..1
            // and probeTime will drive it.
            break;
        }
    }

    if (m_hasData)
        m_timeWidget->show();
}
