#pragma once

#include <QMainWindow>
#include <QMap>
#include <QStringList>
#include "led_matrix_widget.h"

class LcdWidget;
class QSlider;
class QLabel;
class QPushButton;
class QComboBox;

class LcdWindow : public QMainWindow {
    Q_OBJECT
public:
    LcdWindow(const QString &title,
              const QStringList &signalNames,
              const QMap<QString, SignalRef> &signalData,
              QWidget *parent = nullptr);

    void setSignalData(const QMap<QString, SignalRef> &data);
    void setProbeTime(double seconds);

signals:
    void probeTimeChanged(double seconds);

public slots:
    void onTimeSliderChanged(int value);
    void onPlayPause();
    void onSpeedChanged(int index);

private:
    void setupControls();

    LcdWidget *m_widget = nullptr;
    QSlider *m_timeSlider = nullptr;
    QLabel *m_timeLabel = nullptr;
    QWidget *m_controls = nullptr;
    QPushButton *m_playBtn = nullptr;
    QComboBox *m_speedCombo = nullptr;

    double m_timeMin = 0;
    double m_timeMax = 1;
    double m_probeTime = 0;
    bool m_sliderDragging = false;
};
