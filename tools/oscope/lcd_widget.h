#pragma once

#include <QWidget>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QColor>
#include <QFont>
#include <QElapsedTimer>
#include <QTimer>
#include "led_matrix_widget.h"

class LcdWidget : public QWidget {
    Q_OBJECT
public:
    struct DecodedEvent {
        double time;
        bool isCommand;
        uint8_t byte;
    };

    LcdWidget(int cols, int lines, QWidget *parent = nullptr);

    void setSignalData(const QMap<QString, SignalRef> &data);
    void setSignalNames(const QStringList &names);
    void setTime(double seconds);

    void play(double fromTime, double toTime);
    void pause();
    void stop();
    bool isPlaying() const { return m_playing; }
    void setAnimationSpeed(double speed);

    int cols() const { return m_cols; }
    int lines() const { return m_lines; }

signals:
    void timeChanged(double seconds);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    struct RawSample {
        double time;
        bool rs;
        uint8_t nibble;
    };

    void decodeSignals();
    void extractEdges(const QVector<double> &time,
                      const QVector<double> &eVal,
                      const QVector<double> &rsVal,
                      const QVector<double> &d4Val,
                      const QVector<double> &d5Val,
                      const QVector<double> &d6Val,
                      const QVector<double> &d7Val);
    void processEventsUpTo(double time);
    void processEvent(const DecodedEvent &ev);
    void processCommand(uint8_t cmd);
    void processData(uint8_t data);
    void onAnimationTick();

    int m_cols, m_lines;
    QStringList m_signalNames;
    QMap<QString, SignalRef> m_signalData;

    QVector<RawSample> m_samples;
    QVector<DecodedEvent> m_events;
    uint8_t m_ddram[80]{};
    uint8_t m_ddramAddress = 0;
    bool m_displayOn = true;
    bool m_twoLine = true;
    int m_lastEventIndex = 0;

    double m_currentTime = 0;
    double m_animFrom = 0;
    double m_animTo = 1;

    QFont m_charFont;
    QFont m_labelFont;
    bool m_layoutDirty = true;

    bool m_playing = false;
    double m_speed = 2.0;
    QElapsedTimer m_animTimer;
    QTimer *m_animTick = nullptr;

    QColor m_bgColor{"#1a1a2e"};
    QColor m_panelBg{"#3a5a4a"};
    QColor m_cellBg{"#4a6a5a"};
    QColor m_charOn{"#0d1f14"};
    QColor m_charOff{"#5a7a6a"};
    QColor m_borderColor{"#2a3a2e"};
    QColor m_frameColor{"#1a2a2e"};
    QColor m_highlight{"#00f2fe"};
};
