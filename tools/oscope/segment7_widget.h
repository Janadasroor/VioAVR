#pragma once

#include <QWidget>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QPainterPath>
#include <QColor>
#include <QElapsedTimer>
#include <QTimer>

#include "led_matrix_widget.h"  // for SignalRef

class Segment7Widget : public QWidget {
    Q_OBJECT
public:
    explicit Segment7Widget(int numDigits, QWidget *parent = nullptr);

    void setSignalData(const QMap<QString, SignalRef> &data);
    void setSegmentNames(const QStringList &names);
    void setDigitCount(int n);
    void setTime(double seconds);
    double time() const { return m_currentTime; }

    int numDigits() const { return m_numDigits; }

    // Animation
    void play(double fromTime, double toTime);
    void pause();
    void stop();
    bool isPlaying() const { return m_playing; }
    void setAnimationSpeed(double simSecondsPerSecond);
    double animationSpeed() const { return m_speed; }

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

signals:
    void timeChanged(double seconds);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void rebuildDigitPaths();
    double interpolate(const QVector<double> &time, const QVector<double> &values, double t) const;
    QColor segmentColor(double voltage) const;
    void onAnimationTick();

    int m_numDigits;
    QStringList m_segmentNames;  // e.g. ["pb0_an","pb1_an",...] for 7 segments (a-g)
    QMap<QString, SignalRef> m_signalData;
    double m_currentTime = 0.0;

    // 7-segment paths (built once, scaled per render)
    QVector<QPainterPath> m_segPaths;  // 7 paths for a-g
    QRectF m_digitRect;
    qreal m_digitW = 0;
    qreal m_digitH = 0;
    qreal m_thickness = 0;

    // Animation
    bool m_playing = false;
    double m_speed = 1000.0;
    double m_animFrom = 0.0;
    double m_animTo = 1.0;
    QElapsedTimer m_animTimer;
    QTimer *m_animTick = nullptr;

    // Colors
    QColor m_bgColor{"#1a1a2e"};
    QColor m_segOff{"#2a2a4e"};
    QColor m_segOn{"#ff4444"};  // red-orange for classic 7-seg
    QColor m_segDim{"#6b1a1a"};
    QColor m_borderColor{"#3a3a5e"};
};
