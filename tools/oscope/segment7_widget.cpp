#include "segment7_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>
#include <algorithm>

Segment7Widget::Segment7Widget(int numDigits, QWidget *parent)
    : QWidget(parent)
    , m_numDigits(numDigits)
{
    setMinimumSize(60, 100);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_animTick = new QTimer(this);
    m_animTick->setInterval(16);
    connect(m_animTick, &QTimer::timeout, this, &Segment7Widget::onAnimationTick);
    rebuildDigitPaths();
}

void Segment7Widget::setSignalData(const QMap<QString, SignalRef> &data)
{
    m_signalData = data;
    update();
}

void Segment7Widget::setSegmentNames(const QStringList &names)
{
    m_segmentNames = names;
    update();
}

void Segment7Widget::setDigitCount(int n)
{
    m_numDigits = n;
    rebuildDigitPaths();
    update();
}

void Segment7Widget::setTime(double seconds)
{
    m_currentTime = seconds;
    update();
}

QSize Segment7Widget::minimumSizeHint() const
{
    return QSize(m_numDigits * 50 + 10, 90);
}

QSize Segment7Widget::sizeHint() const
{
    return QSize(m_numDigits * 70 + 10, 130);
}

void Segment7Widget::play(double fromTime, double toTime)
{
    m_animFrom = fromTime;
    m_animTo = toTime;
    m_currentTime = fromTime;
    m_playing = true;
    m_animTimer.start();
    if (!m_animTick->isActive())
        m_animTick->start();
}

void Segment7Widget::stop()
{
    m_playing = false;
    m_animTick->stop();
}

void Segment7Widget::pause()
{
    m_playing = false;
    m_animTick->stop();
}

void Segment7Widget::setAnimationSpeed(double simSecondsPerSecond)
{
    m_speed = simSecondsPerSecond;
}

void Segment7Widget::rebuildDigitPaths()
{
    m_segPaths.clear();

    // Normalized coordinates in a 100x160 space
    // This builds the 7 segment paths (a-g) once.
    // We'll scale them in paintEvent based on actual widget size.

    struct SegDef {
        QPointF p1, p2, p3, p4;  // trapezoid corners
    };

    // clang-format off
    // Segment a: top horizontal
    double t = 10;      // thickness
    double gap = 4;     // gap between segments
    double hw = 1.5;    // half-width of trapezoid bevel

    QVector<SegDef> segs(7);
    // a: top horizontal bar
    segs[0] = {{20 - hw, 10}, {80 + hw, 10}, {80 - hw, 10 + t}, {20 + hw, 10 + t}};
    // b: upper-right vertical
    segs[1] = {{80 - t, 25 + hw}, {80, 25 - hw}, {80, 65 + hw}, {80 - t, 65 - hw}};
    // c: lower-right vertical
    segs[2] = {{80 - t, 80 + hw}, {80, 80 - hw}, {80, 120 + hw}, {80 - t, 120 - hw}};
    // d: bottom horizontal
    segs[3] = {{20 + hw, 125 + t}, {80 - hw, 125 + t}, {80 + hw, 125}, {20 - hw, 125}};
    // e: lower-left vertical
    segs[4] = {{10 + t, 80 + hw}, {10 + t, 80 - hw}, {10, 80 - hw}, {10, 120 + hw}};
    // f: upper-left vertical
    segs[5] = {{10, 25 - hw}, {10 + t, 25 + hw}, {10 + t, 65 - hw}, {10, 65 + hw}};
    // g: middle horizontal
    segs[6] = {{20 + hw, 70}, {80 - hw, 70}, {80 + hw, 70 + t}, {20 - hw, 70 + t}};
    // clang-format on

    for (const auto &sd : segs) {
        QPainterPath path;
        path.moveTo(sd.p1);
        path.lineTo(sd.p2);
        path.lineTo(sd.p3);
        path.lineTo(sd.p4);
        path.closeSubpath();
        m_segPaths.append(path);
    }
}

QColor Segment7Widget::segmentColor(double voltage) const
{
    if (voltage > 2.5) {
        return m_segOn;
    } else if (voltage > 0.5) {
        return m_segDim;
    }
    return m_segOff;
}

double Segment7Widget::interpolate(const QVector<double> &time,
                                    const QVector<double> &values,
                                    double t) const
{
    if (time.isEmpty() || values.isEmpty())
        return 0.0;
    if (t <= time.first())
        return values.first();
    if (t >= time.last())
        return values.last();

    auto it = std::upper_bound(time.begin(), time.end(), t);
    int idx = (int)(it - time.begin());
    if (idx <= 0) return values.first();
    if (idx >= time.size()) return values.last();

    double t0 = time[idx - 1];
    double t1 = time[idx];
    double v0 = values[idx - 1];
    double v1 = values[idx];
    double frac = (t - t0) / (t1 - t0);
    return v0 + frac * (v1 - v0);
}

void Segment7Widget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), m_bgColor);

    if (m_segmentNames.isEmpty()) {
        p.setPen(QColor("#cbd5e1"));
        p.setFont(QFont("sans-serif", 10));
        p.drawText(rect(), Qt::AlignCenter, "No 7-segment signals detected");
        return;
    }

    p.setPen(QPen(m_borderColor, 1));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);

    // Time label
    p.setPen(QColor("#cbd5e1"));
    p.setFont(QFont("monospace", 9));
    p.drawText(QRect(4, 2, width() - 8, 14), Qt::AlignRight,
               QString("t = %1 s").arg(m_currentTime, 0, 'e', 3));

    // Compute digit width and spacing
    qreal totalW = width() - 8;
    qreal totalH = height() - 24;
    qreal digitW = totalW / m_numDigits;
    qreal digitH = totalH;

    // Scale the normalized paths
    // Normalized 100x160 space
    double scaleX = digitW / 100.0;
    double scaleY = digitH / 170.0;
    double scale = std::min(scaleX, scaleY);

    double digCenterX = digitW / 2.0;
    double digTopY = (totalH - 170.0 * scale) / 2.0 + 24;

    for (int di = 0; di < m_numDigits; ++di) {
        double ox = 4 + di * digitW + digCenterX - 50.0 * scale;
        double oy = digTopY;

        // Read voltage for each segment
        double v[7] = {};
        for (int s = 0; s < 7 && s < m_segmentNames.size(); ++s) {
            int idx = di * 7 + s;
            if (idx < m_segmentNames.size()) {
                auto it = m_signalData.find(m_segmentNames[idx]);
                if (it != m_signalData.end() && it->time && it->values) {
                    v[s] = interpolate(*it->time, *it->values, m_currentTime);
                }
            }
        }

        // Draw segments
        for (int s = 0; s < 7 && s < m_segPaths.size(); ++s) {
            QPainterPath sp = m_segPaths[s];

            // Transform: scale and translate to position
            QTransform tf;
            tf.translate(ox, oy);
            tf.scale(scale, scale);
            sp = tf.map(sp);

            QColor color = segmentColor(v[s]);

            // Glow layer: slightly larger semi-transparent copy behind lit segments
            if (v[s] > 2.5) {
                p.save();
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(255, 60, 60, 40));
                QPainterPath glow = sp.translated(0, 1.5);
                glow += sp.translated(0, -1.5);
                glow += sp.translated(1.5, 0);
                glow += sp.translated(-1.5, 0);
                p.drawPath(glow);
                p.restore();
            }

            p.setPen(QPen(m_borderColor, 1));
            p.setBrush(color);
            p.drawPath(sp);
        }
    }
}

void Segment7Widget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void Segment7Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit timeChanged(m_currentTime);
    }
    QWidget::mousePressEvent(event);
}

void Segment7Widget::onAnimationTick()
{
    if (!m_playing) return;

    qreal elapsed = m_animTimer.elapsed() / 1000.0;
    m_animTimer.restart();

    qreal range = m_animTo - m_animFrom;
    qreal dt = elapsed * m_speed * range;
    m_currentTime += dt;
    if (m_currentTime >= m_animTo) {
        m_currentTime = m_animFrom + std::fmod(m_currentTime - m_animFrom, range);
    }
    emit timeChanged(m_currentTime);
    update();
}
