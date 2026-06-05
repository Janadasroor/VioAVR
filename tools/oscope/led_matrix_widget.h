#pragma once

#include <QWidget>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QColor>
#include <QElapsedTimer>
#include <QTimer>

struct SignalRef {
    const QVector<double> *time = nullptr;
    const QVector<double> *values = nullptr;
};

class LedMatrixWidget : public QWidget {
    Q_OBJECT
public:
    explicit LedMatrixWidget(int rows, int cols, QWidget *parent = nullptr);

    void setSignalData(const QMap<QString, SignalRef> &data);
    void setSignalNames(const QStringList &names);
    void setMatrixMode(const QStringList &rowSignals, const QStringList &colSignals);
    void setTime(double seconds);
    double time() const { return m_currentTime; }

    int rows() const { return m_rows; }
    int cols() const { return m_cols; }
    const QStringList& signalNames() const { return m_signalNames; }

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    // Animation
    void play(double fromTime, double toTime);
    void pause();
    bool isPlaying() const { return m_playing; }
    void setAnimationSpeed(double simSecondsPerSecond);
    double animationSpeed() const { return m_speed; }

signals:
    void timeChanged(double seconds);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildLayout();
    double interpolate(const QVector<double> &time, const QVector<double> &values, double t) const;
    double computeLedBrightness(int row, int col) const;
    void onAnimationTick();

    int m_rows;
    int m_cols;
    QStringList m_signalNames;
    QMap<QString, SignalRef> m_signalData;
    double m_currentTime = 0.0;

    // Matrix mode (row×col decode)
    bool m_isMatrix = false;
    QStringList m_rowSignals;
    QStringList m_colSignals;

    // Layout cache
    QVector<QRectF> m_ledRects;
    QVector<QString> m_ledLabels;
    QFont m_ledFont;
    qreal m_cellW = 0;
    qreal m_cellH = 0;
    qreal m_ledRadius = 0;

    // Animation
    bool m_playing = false;
    double m_speed = 1000.0;
    double m_animFrom = 0.0;
    double m_animTo = 1.0;
    QElapsedTimer m_animTimer;
    QTimer *m_animTick = nullptr;

    // Colors
    QColor m_bgColor{"#1a1a2e"};
    QColor m_ledOff{"#2a2a4e"};
    QColor m_ledOn{"#00ff88"};
    QColor m_ledDim{"#1a6b3e"};
    QColor m_labelColor{"#cbd5e1"};
    QColor m_borderColor{"#3a3a5e"};
};
