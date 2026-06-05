#pragma once

#include <QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QVector>

#include "segment7_widget.h"

class Segment7Window : public QMainWindow {
    Q_OBJECT
public:
    explicit Segment7Window(const QString &title,
                            const QStringList &segmentSignalNames,
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

    Segment7Widget *m_segWidget;

    QWidget *m_controls;
    QSlider *m_timeSlider;
    QLabel *m_timeLabel;
    QPushButton *m_playBtn;
    QComboBox *m_speedCombo;

    double m_probeTime = 0.0;
    double m_timeMin = 0.0;
    double m_timeMax = 1.0;
    bool m_sliderDragging = false;
};
