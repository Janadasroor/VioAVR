#include "led_matrix_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>

LedMatrixWindow::LedMatrixWindow(const QString &title,
                                 const QStringList &rowSignals,
                                 const QStringList &colSignals,
                                 const QMap<QString, SignalRef> &signalData,
                                 QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(title);
    resize(640, 540);
    setAttribute(Qt::WA_DeleteOnClose, false);

    // Compute time range from signal data
    m_timeMin = 0.0;
    m_timeMax = 1.0;
    bool foundRange = false;
    for (auto it = signalData.begin(); it != signalData.end(); ++it) {
        if (it->time && !it->time->isEmpty()) {
            double t0 = it->time->first();
            double t1 = it->time->last();
            if (!foundRange) {
                m_timeMin = t0;
                m_timeMax = t1;
                foundRange = true;
            } else {
                m_timeMin = std::min(m_timeMin, t0);
                m_timeMax = std::max(m_timeMax, t1);
            }
        }
    }

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_matrix = new LedMatrixWidget(10, 10);
    m_matrix->setMatrixMode(rowSignals, colSignals);
    m_matrix->setSignalData(signalData);
    layout->addWidget(m_matrix, 1);

    setupControls();
    layout->addWidget(m_controls);

    setCentralWidget(central);

    statusBar()->showMessage(
        QString("Time range: %1 s – %2 s").arg(m_timeMin, 0, 'e', 3).arg(m_timeMax, 0, 'e', 3));

    connect(m_matrix, &LedMatrixWidget::timeChanged, this, [this](double t) {
        setProbeTime(t);
    });
}

void LedMatrixWindow::setupControls()
{
    m_controls = new QWidget;
    auto *layout = new QVBoxLayout(m_controls);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    auto *row1 = new QHBoxLayout;
    m_playBtn = new QPushButton("\u25B6");
    m_playBtn->setFixedSize(28, 24);
    m_playBtn->setCheckable(true);
    m_playBtn->setToolTip("Play/Pause animation");
    m_playBtn->setStyleSheet(
        "QPushButton { background: #2a2a4e; color: #00f2fe; border: 1px solid #3a3a5e;"
        "  border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background: #3a3a5e; }"
        "QPushButton:checked { background: #1a4a3e; color: #ff6b6b; }");

    m_speedCombo = new QComboBox;
    m_speedCombo->addItem("1\u00D7", 1.0);
    m_speedCombo->addItem("10\u00D7", 10.0);
    m_speedCombo->addItem("100\u00D7", 100.0);
    m_speedCombo->addItem("1000\u00D7", 1000.0);
    m_speedCombo->addItem("10000\u00D7", 10000.0);
    m_speedCombo->setCurrentIndex(2);
    m_speedCombo->setStyleSheet(
        "QComboBox { background: #2a2a4e; color: #cbd5e1; border: 1px solid #3a3a5e;"
        "  border-radius: 4px; padding: 2px 6px; font-size: 10px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: #1a1a2e; color: #cbd5e1;"
        "  selection-background-color: #3a3a5e; }");

    row1->addWidget(m_playBtn);
    row1->addWidget(m_speedCombo, 1);
    layout->addLayout(row1);

    m_timeSlider = new QSlider(Qt::Horizontal);
    m_timeSlider->setRange(0, 10000);
    m_timeSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 4px; background: #2a2a4e; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #00f2fe; width: 12px; height: 12px;"
        "  margin: -4px 0; border-radius: 6px; }"
        "QSlider::sub-page:horizontal { background: #4facfe; border-radius: 2px; }");
    layout->addWidget(m_timeSlider);

    m_timeLabel = new QLabel("t = 0 s");
    m_timeLabel->setStyleSheet("color: #94a3b8; font-size: 10px; font-family: monospace;");
    layout->addWidget(m_timeLabel);

    // Connections
    connect(m_timeSlider, &QSlider::sliderPressed, this, [this]() {
        m_sliderDragging = true;
    });
    connect(m_timeSlider, &QSlider::sliderReleased, this, [this]() {
        m_sliderDragging = false;
        onTimeSliderChanged(m_timeSlider->value());
    });
    connect(m_timeSlider, &QSlider::valueChanged, this, [this](int) {
        if (m_sliderDragging)
            onTimeSliderChanged(m_timeSlider->value());
    });
    connect(m_playBtn, &QPushButton::toggled, this, &LedMatrixWindow::onPlayPause);
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LedMatrixWindow::onSpeedChanged);
}

void LedMatrixWindow::setSignalData(const QMap<QString, SignalRef> &data)
{
    m_matrix->setSignalData(data);
}

void LedMatrixWindow::setProbeTime(double seconds)
{
    m_probeTime = seconds;
    m_matrix->setTime(seconds);

    if (!m_sliderDragging && m_timeMax > m_timeMin) {
        double frac = (seconds - m_timeMin) / (m_timeMax - m_timeMin);
        m_timeSlider->blockSignals(true);
        m_timeSlider->setValue((int)(frac * 10000));
        m_timeSlider->blockSignals(false);
    }

    m_timeLabel->setText(QString("t = %1 s").arg(seconds, 0, 'e', 3));
}

void LedMatrixWindow::onTimeSliderChanged(int value)
{
    if (m_timeMax <= m_timeMin) return;
    double frac = value / 10000.0;
    double t = m_timeMin + frac * (m_timeMax - m_timeMin);
    setProbeTime(t);
    emit probeTimeChanged(t);
}

void LedMatrixWindow::onPlayPause()
{
    if (m_playBtn->isChecked()) {
        m_matrix->play(m_probeTime, m_timeMax);
    } else {
        m_matrix->pause();
    }
}

void LedMatrixWindow::onSpeedChanged(int index)
{
    double speed = m_speedCombo->itemData(index).toDouble();
    m_matrix->setAnimationSpeed(speed);
}
