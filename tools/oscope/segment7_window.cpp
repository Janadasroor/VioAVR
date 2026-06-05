#include "segment7_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>

Segment7Window::Segment7Window(const QString &title,
                               const QStringList &segmentSignalNames,
                               const QMap<QString, SignalRef> &signalData,
                               QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(title);
    resize(500, 280);
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

    // Determine number of digits from signal count
    int numDigits = segmentSignalNames.size() / 7;
    if (numDigits < 1) numDigits = 1;

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_segWidget = new Segment7Widget(numDigits);
    m_segWidget->setSegmentNames(segmentSignalNames);
    m_segWidget->setSignalData(signalData);
    layout->addWidget(m_segWidget, 1);

    setupControls();
    layout->addWidget(m_controls);

    setCentralWidget(central);

    statusBar()->showMessage(
        QString("%1-digit 7-segment, time: %2 s \u2013 %3 s")
            .arg(numDigits)
            .arg(m_timeMin, 0, 'e', 3)
            .arg(m_timeMax, 0, 'e', 3));

    connect(m_segWidget, &Segment7Widget::timeChanged, this, [this](double t) {
        setProbeTime(t);
    });

    // Auto-start animation
    m_playBtn->setChecked(true);
    m_segWidget->play(m_timeMin, m_timeMax);
}

void Segment7Window::setupControls()
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
    m_speedCombo->addItem("0.5 Hz", 0.5);
    m_speedCombo->addItem("1 Hz", 1.0);
    m_speedCombo->addItem("2 Hz", 2.0);
    m_speedCombo->addItem("5 Hz", 5.0);
    m_speedCombo->addItem("10 Hz", 10.0);
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
    connect(m_playBtn, &QPushButton::toggled, this, &Segment7Window::onPlayPause);
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Segment7Window::onSpeedChanged);
}

void Segment7Window::setSignalData(const QMap<QString, SignalRef> &data)
{
    m_segWidget->setSignalData(data);
}

void Segment7Window::setProbeTime(double seconds)
{
    m_probeTime = seconds;
    m_segWidget->setTime(seconds);

    if (!m_sliderDragging && m_timeMax > m_timeMin) {
        double frac = (seconds - m_timeMin) / (m_timeMax - m_timeMin);
        m_timeSlider->blockSignals(true);
        m_timeSlider->setValue((int)(frac * 10000));
        m_timeSlider->blockSignals(false);
    }

    m_timeLabel->setText(QString("t = %1 s").arg(seconds, 0, 'e', 3));
}

void Segment7Window::onTimeSliderChanged(int value)
{
    if (m_timeMax <= m_timeMin) return;
    double frac = value / 10000.0;
    double t = m_timeMin + frac * (m_timeMax - m_timeMin);
    setProbeTime(t);
    emit probeTimeChanged(t);
}

void Segment7Window::onPlayPause()
{
    if (m_playBtn->isChecked()) {
        m_segWidget->play(m_probeTime, m_timeMax);
    } else {
        m_segWidget->pause();
    }
}

void Segment7Window::onSpeedChanged(int index)
{
    double speed = m_speedCombo->itemData(index).toDouble();
    m_segWidget->setAnimationSpeed(speed);
}
