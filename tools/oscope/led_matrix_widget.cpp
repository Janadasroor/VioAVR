#include "led_matrix_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>
#include <algorithm>

LedMatrixWidget::LedMatrixWidget(int rows, int cols, QWidget *parent)
    : QWidget(parent)
    , m_rows(rows)
    , m_cols(cols)
{
    setMinimumSize(200, 60);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_ledFont = QFont("monospace", 8);
    m_animTick = new QTimer(this);
    m_animTick->setInterval(30); // ~33 fps
    connect(m_animTick, &QTimer::timeout, this, &LedMatrixWidget::onAnimationTick);
}

void LedMatrixWidget::setSignalData(const QMap<QString, SignalRef> &data)
{
    m_signalData = data;
    update();
}

void LedMatrixWidget::setSignalNames(const QStringList &names)
{
    m_isMatrix = false;
    m_signalNames = names;
    rebuildLayout();
    update();
}

void LedMatrixWidget::setMatrixMode(const QStringList &rowSignals, const QStringList &colSignals)
{
    m_isMatrix = true;
    m_rowSignals = rowSignals;
    m_colSignals = colSignals;
    // Compose signalNames from row+col for rebuildLayout
    m_signalNames = rowSignals + colSignals;
    rebuildLayout();
    update();
}

void LedMatrixWidget::setTime(double seconds)
{
    m_currentTime = seconds;
    update();
}

QSize LedMatrixWidget::minimumSizeHint() const
{
    return QSize(m_cols * 32 + 20, m_rows * 36 + 24);
}

QSize LedMatrixWidget::sizeHint() const
{
    return QSize(m_cols * 40 + 20, m_rows * 44 + 24);
}

void LedMatrixWidget::play(double fromTime, double toTime)
{
    m_animFrom = fromTime;
    m_animTo = toTime;
    m_currentTime = fromTime;
    m_playing = true;
    m_animTimer.start();
    if (!m_animTick->isActive())
        m_animTick->start();
}

void LedMatrixWidget::pause()
{
    m_playing = false;
    m_animTick->stop();
}

void LedMatrixWidget::setAnimationSpeed(double simSecondsPerSecond)
{
    m_speed = simSecondsPerSecond;
}

void LedMatrixWidget::rebuildLayout()
{
    m_ledRects.clear();
    m_ledLabels.clear();

    if (m_isMatrix) {
        // Matrix mode: build a rows × cols grid
        // Each cell labeled R{r}C{c}
        qreal w = width() - 8;
        qreal h = height() - 8;
        m_cellW = w / m_cols;
        m_cellH = h / m_rows;
        m_ledRadius = std::min(m_cellW, m_cellH) * 0.35;

        for (int r = 0; r < m_rows; ++r) {
            for (int c = 0; c < m_cols; ++c) {
                qreal cx = 4 + c * m_cellW + m_cellW / 2;
                qreal cy = 4 + r * m_cellH + m_cellH / 2;
                m_ledRects.append(QRectF(cx - m_ledRadius, cy - m_ledRadius,
                                         m_ledRadius * 2, m_ledRadius * 2));
                m_ledLabels.append(QString("R%1C%2").arg(r).arg(c));
            }
        }
    } else {
        // Direct mode: one signal per LED (pin status bar)
        if (m_signalNames.isEmpty()) return;

        qreal w = width() - 8;
        qreal h = height() - 8;
        m_cellW = w / m_cols;
        m_cellH = h / m_rows;
        m_ledRadius = std::min(m_cellW, m_cellH) * 0.3;

        for (int r = 0; r < m_rows; ++r) {
            for (int c = 0; c < m_cols; ++c) {
                int idx = r * m_cols + c;
                if (idx < m_signalNames.size()) {
                    qreal cx = 4 + c * m_cellW + m_cellW / 2;
                    qreal cy = 4 + r * m_cellH + m_cellH / 2;
                    m_ledRects.append(QRectF(cx - m_ledRadius, cy - m_ledRadius,
                                             m_ledRadius * 2, m_ledRadius * 2));
                    QString lbl = m_signalNames[idx];
                    lbl = lbl.remove("_an").toUpper();
                    m_ledLabels.append(lbl);
                }
            }
        }
    }

    int fontSize = std::max(6, (int)(m_ledRadius * 0.6));
    m_ledFont = QFont("monospace", fontSize);
}

double LedMatrixWidget::interpolate(const QVector<double> &time,
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

double LedMatrixWidget::computeLedBrightness(int row, int col) const
{
    if (!m_isMatrix)
        return 0.0;

    // Row signal: get voltage on row pin
    double vRow = 0.0;
    if (row < m_rowSignals.size()) {
        auto it = m_signalData.find(m_rowSignals[row]);
        if (it != m_signalData.end() && it->time && it->values) {
            vRow = interpolate(*it->time, *it->values, m_currentTime);
        }
    }

    // Column signal: get voltage on column pin
    double vCol = 0.0;
    if (col < m_colSignals.size()) {
        auto it = m_signalData.find(m_colSignals[col]);
        if (it != m_signalData.end() && it->time && it->values) {
            vCol = interpolate(*it->time, *it->values, m_currentTime);
        }
    }

    // LED at (row, col) is ON when row is high (> 2.5V) and column is low (< 0.5V)
    if (vRow > 2.5 && vCol < 0.5) {
        double intensity = std::min(1.0, (vRow - 2.5) / 2.5);
        return intensity;
    } else if (vRow > 2.5 && vCol < 2.5) {
        // Row on, column partially on — dim
        double fracCol = (2.5 - vCol) / 2.0;
        return 0.3 * fracCol;
    }

    return 0.0;
}

void LedMatrixWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), m_bgColor);

    if (m_signalNames.isEmpty() && !m_isMatrix) {
        p.setPen(m_labelColor);
        p.setFont(QFont("sans-serif", 10));
        p.drawText(rect(), Qt::AlignCenter, "No pin signals detected");
        return;
    }

    // Border
    p.setPen(QPen(m_borderColor, 1));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);

    // Time label
    p.setPen(m_labelColor);
    p.setFont(QFont("monospace", 9));
    p.drawText(QRect(4, 2, width() - 8, 14), Qt::AlignRight,
               QString("t = %1 s").arg(m_currentTime, 0, 'e', 3));

    if (m_isMatrix) {
        // Matrix mode: compute each LED from row×col state
        for (int r = 0; r < m_rows; ++r) {
            for (int c = 0; c < m_cols; ++c) {
                int idx = r * m_cols + c;
                if (idx >= m_ledRects.size()) continue;

                const QRectF &rect = m_ledRects[idx];
                double brightness = computeLedBrightness(r, c);

                QColor ledColor;
                if (brightness > 0.5) {
                    ledColor = QColor::fromRgbF(0.0, 0.5 + 0.5 * brightness, 0.2 * brightness);
                } else if (brightness > 0.1) {
                    ledColor = QColor::fromRgbF(0.05, 0.15 + 0.3 * brightness, 0.0);
                } else {
                    ledColor = m_ledOff;
                }

                // Draw with glow
                QRadialGradient grad(rect.center(), rect.width() * 0.7);
                if (brightness > 0.5) {
                    grad.setColorAt(0.0, QColor::fromRgbF(0.3, 1.0, 0.5));
                    grad.setColorAt(0.6, ledColor);
                    grad.setColorAt(1.0, m_bgColor);
                } else {
                    grad.setColorAt(0.0, ledColor);
                    grad.setColorAt(1.0, m_bgColor);
                }
                p.setBrush(grad);
                p.setPen(QPen(m_borderColor, 1));
                p.drawEllipse(rect);

                // Label
                p.setPen(m_labelColor);
                p.setFont(m_ledFont);
                QRectF labelRect(rect.x() - m_cellW / 2 + 2,
                                 rect.bottom() + 2,
                                 m_cellW - 4, 14);
                p.drawText(labelRect, Qt::AlignCenter, m_ledLabels[idx]);
            }
        }
    } else {
        // Direct mode: one signal per LED
        for (int idx = 0; idx < m_signalNames.size() && idx < m_ledRects.size(); ++idx) {
            const QString &sigName = m_signalNames[idx];
            const QRectF &rect = m_ledRects[idx];

            double v = 0.0;
            auto it = m_signalData.find(sigName);
            if (it != m_signalData.end() && it->time && it->values) {
                v = interpolate(*it->time, *it->values, m_currentTime);
            }

            QColor ledColor;
            if (v > 2.5) {
                double intensity = std::min(1.0, (v - 2.5) / 2.5);
                ledColor = QColor::fromRgbF(0.0, 0.5 + 0.5 * intensity, 0.2 * intensity);
            } else if (v < 0.5) {
                ledColor = m_ledOff;
            } else {
                double frac = (v - 0.5) / 2.0;
                ledColor = QColor::fromRgbF(0.1 * frac, 0.1 + 0.4 * frac, 0.0);
            }

            QRadialGradient grad(rect.center(), rect.width() * 0.7);
            if (v > 2.5) {
                grad.setColorAt(0.0, QColor::fromRgbF(0.3, 1.0, 0.5));
                grad.setColorAt(0.6, ledColor);
                grad.setColorAt(1.0, m_bgColor);
            } else {
                grad.setColorAt(0.0, ledColor);
                grad.setColorAt(1.0, m_bgColor);
            }
            p.setBrush(grad);
            p.setPen(QPen(m_borderColor, 1));
            p.drawEllipse(rect);

            p.setPen(m_labelColor);
            p.setFont(m_ledFont);
            QRectF labelRect(rect.x() - m_cellW / 2 + 2,
                             rect.bottom() + 2,
                             m_cellW - 4, 14);
            p.drawText(labelRect, Qt::AlignCenter, m_ledLabels[idx]);
        }
    }
}

void LedMatrixWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        for (int i = 0; i < m_ledRects.size(); ++i) {
            if (m_ledRects[i].contains(event->pos())) {
                emit timeChanged(m_currentTime);
                break;
            }
        }
    }
    QWidget::mousePressEvent(event);
}

void LedMatrixWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    rebuildLayout();
}

void LedMatrixWidget::onAnimationTick()
{
    if (!m_playing) return;

    qreal elapsed = m_animTimer.elapsed() / 1000.0;
    m_animTimer.restart();

    // Speed: 1× = full range plays in 5 real seconds
    qreal range = m_animTo - m_animFrom;
    qreal totalRealSeconds = 5.0 / m_speed;
    qreal dt = (elapsed / totalRealSeconds) * range;

    m_currentTime += dt;
    if (m_currentTime >= m_animTo) {
        m_currentTime = m_animTo;
        m_playing = false;
        m_animTick->stop();
    }
    emit timeChanged(m_currentTime);
    update();
}
