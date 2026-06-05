#pragma once

#include <QDockWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QMap>
#include <QVector>

#include "led_matrix_widget.h"

struct InstrumentDef;
struct SignalRef;

class InstrumentPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit InstrumentPanel(QWidget *parent = nullptr);

    void clearInstruments();
    void addLedMatrix(const InstrumentDef &def,
                      const QMap<QString, SignalRef> &signalData);
    void setSignalData(const QMap<QString, SignalRef> &data);

    // Time synchronization
    void setProbeTime(double seconds);
    double probeTime() const { return m_probeTime; }

signals:
    void probeTimeChanged(double seconds);

private slots:
    void onSliderChanged(int value);
    void onPlayPause();
    void onSpeedChanged(int index);
    void onInstrumentTimeChanged(double t);

private:
    void rebuildTimeControls();

    QWidget *m_container;
    QVBoxLayout *m_layout;
    QLabel *m_header;
    QLabel *m_emptyLabel;
    QScrollArea *m_scroll;

    // Time controls
    QWidget *m_timeWidget;
    QSlider *m_timeSlider;
    QLabel *m_timeLabel;
    QPushButton *m_playBtn;
    QComboBox *m_speedCombo;

    double m_probeTime = 0.0;
    double m_timeMin = 0.0;
    double m_timeMax = 1.0;
    bool m_sliderDragging = false;
    bool m_following = true;

    QMap<QString, LedMatrixWidget*> m_instruments;
    bool m_hasData = false;
};
