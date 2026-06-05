#include "lcd_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>
#include <algorithm>

LcdWidget::LcdWidget(int cols, int lines, QWidget *parent)
    : QWidget(parent)
    , m_cols(cols)
    , m_lines(lines)
{
    setMinimumSize(cols * 20 + 10, lines * 40 + 10);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_charFont = QFont("monospace", 14);
    m_charFont.setBold(true);
    m_labelFont = QFont("monospace", 9);
    m_animTick = new QTimer(this);
    m_animTick->setInterval(16);
    connect(m_animTick, &QTimer::timeout, this, &LcdWidget::onAnimationTick);
}

void LcdWidget::setSignalData(const QMap<QString, SignalRef> &data)
{
    m_signalData = data;
    m_events.clear();
    m_samples.clear();
    m_lastEventIndex = 0;
    decodeSignals();
    processEventsUpTo(m_currentTime);
    update();
}

void LcdWidget::setSignalNames(const QStringList &names)
{
    m_signalNames = names;
}

void LcdWidget::setTime(double seconds)
{
    m_currentTime = seconds;
    processEventsUpTo(seconds);
    update();
}

void LcdWidget::play(double fromTime, double toTime)
{
    m_animFrom = fromTime;
    m_animTo = toTime;
    m_currentTime = fromTime;
    m_lastEventIndex = 0;
    std::fill(m_ddram, m_ddram + 80, 0);
    m_ddramAddress = 0;
    m_displayOn = true;
    m_playing = true;
    m_animTimer.start();
    if (!m_animTick->isActive())
        m_animTick->start();
}

void LcdWidget::pause()
{
    m_playing = false;
    m_animTick->stop();
}

void LcdWidget::stop()
{
    m_playing = false;
    m_animTick->stop();
}

void LcdWidget::setAnimationSpeed(double speed)
{
    m_speed = speed;
}

// ─── HD44780 Protocol Decoder ─────────────────────────────────────

void LcdWidget::extractEdges(const QVector<double> &time,
                             const QVector<double> &eVal,
                             const QVector<double> &rsVal,
                             const QVector<double> &d4Val,
                             const QVector<double> &d5Val,
                             const QVector<double> &d6Val,
                             const QVector<double> &d7Val)
{
    m_samples.clear();
    // Capture on E FALLING edge. HD44780 latches data as E transitions
    // from HIGH to LOW. Read nibble from the last stable HIGH period.
    bool eWasHigh = false;
    uint8_t stableNibble = 0xFF;
    bool stableRs = false;

    auto getNibble = [&](int idx) -> uint8_t {
        uint8_t n = 0;
        if (idx < d4Val.size() && d4Val[idx] > 2.0) n |= 0x10;
        if (idx < d5Val.size() && d5Val[idx] > 2.0) n |= 0x20;
        if (idx < d6Val.size() && d6Val[idx] > 2.0) n |= 0x40;
        if (idx < d7Val.size() && d7Val[idx] > 2.0) n |= 0x80;
        return n;
    };

    for (int i = 0; i < time.size() && i < eVal.size(); ++i) {
        double e = eVal[i];

        if (e > 4.5) {
            stableNibble = getNibble(i);
            stableRs = (i < rsVal.size() && rsVal[i] > 2.0);
            eWasHigh = true;
        }

        if (eWasHigh && e < 0.8) {
            // Falling edge detected
            if (i > 0 && time[i] > 1e-9 && stableNibble != 0xFF) {
                RawSample s;
                s.time = time[i] - 1e-9;
                s.rs = stableRs;
                s.nibble = stableNibble;
                m_samples.append(s);
                if (m_samples.size() == 1 && stableNibble == 0x00) {
                    m_samples.clear();
                }
            }
            eWasHigh = false;
        }
    }
}

void LcdWidget::decodeSignals()
{
    auto rsIt = m_signalData.find(m_signalNames[0]);
    auto eIt  = m_signalData.find(m_signalNames[1]);
    auto d4It = m_signalData.find(m_signalNames[2]);
    auto d5It = m_signalData.find(m_signalNames[3]);
    auto d6It = m_signalData.find(m_signalNames[4]);
    auto d7It = m_signalData.find(m_signalNames[5]);

    if (rsIt == m_signalData.end() || eIt == m_signalData.end() ||
        d4It == m_signalData.end() || d5It == m_signalData.end() ||
        d6It == m_signalData.end() || d7It == m_signalData.end())
        return;

    if (!eIt->time || !eIt->values) return;

    extractEdges(*eIt->time, *eIt->values,
                 *rsIt->values, *d4It->values,
                 *d5It->values, *d6It->values, *d7It->values);

    if (m_samples.isEmpty()) return;

    // Pair nibbles into bytes using time-gap heuristic.
    // Gap < 10µs → same byte, Gap >= 10µs → new byte (warm-up).
    m_events.clear();
    bool inHighNibble = true;
    uint8_t highNibble = 0;
    double lastTime = 0;
    int warmupCount = 0;

    for (const auto &s : m_samples) {
        if (inHighNibble) {
            highNibble = s.nibble;
            lastTime = s.time;
            inHighNibble = false;
        } else {
            double gap = s.time - lastTime;
            if (gap < 10e-6) {
                // Same byte: high nibble + low nibble
                uint8_t byte = highNibble | (s.nibble >> 4);
                // Skip warm-up (first 4 single-nibble commands from HD44780 init)
                if (warmupCount < 2) {
                    warmupCount++;
                    inHighNibble = true;
                    continue;
                }
                DecodedEvent ev;
                ev.time = s.time;
                ev.isCommand = !s.rs;
                ev.byte = byte;
                m_events.append(ev);
                inHighNibble = true;
            } else {
                // New byte start (warm-up or single-nibble command)
                warmupCount++;
                if (warmupCount >= 2) {
                    DecodedEvent ev;
                    ev.time = lastTime;
                    ev.isCommand = true;
                    ev.byte = highNibble;
                    m_events.append(ev);
                }
                highNibble = s.nibble;
                lastTime = s.time;
                inHighNibble = false;
            }
        }
    }
}

void LcdWidget::processEventsUpTo(double time)
{
    while (m_lastEventIndex < m_events.size() &&
           m_events[m_lastEventIndex].time <= time) {
        processEvent(m_events[m_lastEventIndex]);
        m_lastEventIndex++;
    }
}

void LcdWidget::processEvent(const DecodedEvent &ev)
{
    if (ev.isCommand)
        processCommand(ev.byte);
    else
        processData(ev.byte);
}

void LcdWidget::processCommand(uint8_t cmd)
{
    switch (cmd >> 4) {
    case 0x0:
        if (cmd == 0x01) {
            std::fill(m_ddram, m_ddram + 80, 0x20);
            m_ddramAddress = 0;
        } else if (cmd == 0x02) {
            m_ddramAddress = 0;
        } else if ((cmd & 0xF8) == 0x08) {
            m_displayOn = (cmd & 0x04) != 0;
        }
        break;
    case 0x2:
        m_twoLine = (cmd & 0x08) != 0;
        break;
    case 0x8:
        if (cmd >= 0x80)
            m_ddramAddress = cmd & 0x7F;
        break;
    }
}

void LcdWidget::processData(uint8_t data)
{
    if (m_ddramAddress < 80) {
        m_ddram[m_ddramAddress] = data;
    }
    m_ddramAddress++;
    if (m_ddramAddress >= 80)
        m_ddramAddress = 0;
}

// ─── Painting ─────────────────────────────────────────────────────

void LcdWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_layoutDirty = true;
}

void LcdWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), m_bgColor);

    if (m_signalNames.isEmpty()) {
        p.setPen(QColor("#cbd5e1"));
        p.setFont(m_labelFont);
        p.drawText(rect(), Qt::AlignCenter, "No LCD signals detected");
        return;
    }

    qreal w = width();
    qreal h = height();

    // ── Frame/bezel ──
    QRectF frame(2, 2, w - 4, h - 4);
    p.setPen(QPen(m_frameColor, 1));
    p.setBrush(m_frameColor);
    p.drawRoundedRect(frame, 3, 3);

    p.setPen(QPen(QColor("#3a3a5e"), 1));
    p.drawLine(frame.topLeft(), frame.topRight());
    p.drawLine(frame.topLeft(), frame.bottomLeft());
    p.setPen(QPen(QColor("#0a0a1e"), 1));
    p.drawLine(frame.bottomLeft(), frame.bottomRight());
    p.drawLine(frame.topRight(), frame.bottomRight());

    // ── LCD panel ──
    qreal margin = 6;
    QRectF panel(frame.x() + margin, frame.y() + margin,
                 frame.width() - 2 * margin, frame.height() - 2 * margin - 16);

    QPainterPath panelPath;
    panelPath.addRoundedRect(panel, 2, 2);
    p.setPen(QPen(m_borderColor, 1));
    p.setBrush(m_panelBg);
    p.drawPath(panelPath);

    p.setPen(QPen(QColor(0, 0, 0, 60), 1));
    p.drawLine(panel.topLeft(), panel.topRight());
    p.drawLine(panel.topLeft(), panel.bottomLeft());

    // ── Time label ──
    p.setPen(QColor("#94a3b8"));
    p.setFont(m_labelFont);
    p.drawText(QRectF(4, h - 14, w - 8, 12), Qt::AlignRight,
               QString("t = %1 s").arg(m_currentTime, 0, 'e', 3));

    if (m_cols <= 0 || m_lines <= 0) return;

    // ── Character cells ──
    qreal cw = panel.width() / m_cols;
    qreal ch = panel.height() / m_lines;

    int fontSize = (int)std::min(cw * 0.65, ch * 0.7);
    if (fontSize < 6) fontSize = 6;
    if (fontSize > 40) fontSize = 40;
    m_charFont.setPixelSize(fontSize);
    p.setFont(m_charFont);

    int lineStart[2] = {0, 0x40};

    for (int row = 0; row < m_lines && row < 2; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            QRectF cell(panel.x() + col * cw, panel.y() + row * ch, cw, ch);

            QPainterPath cellPath;
            cellPath.addRoundedRect(cell.adjusted(0.5, 0.5, -0.5, -0.5), 1, 1);
            p.setPen(Qt::NoPen);
            p.setBrush(m_cellBg);
            p.drawPath(cellPath);

            if (!m_displayOn)
                continue;

            int addr = lineStart[row] + col;
            char c = (addr < 80) ? (char)m_ddram[addr] : ' ';
            if (c == 0) c = ' ';

            p.setPen((c != ' ') ? m_charOn : m_charOff);
            p.drawText(cell, Qt::AlignCenter, QString(c));
        }
    }

    // ── Grid lines ──
    p.setPen(QPen(QColor(0, 0, 0, 30), 0.5));
    for (int col = 1; col < m_cols; ++col) {
        qreal x = panel.x() + col * cw;
        p.drawLine(QPointF(x, panel.y() + 2), QPointF(x, panel.bottom() - 2));
    }
    for (int row = 1; row < m_lines; ++row) {
        qreal y = panel.y() + row * ch;
        p.drawLine(QPointF(panel.x() + 2, y), QPointF(panel.right() - 2, y));
    }
}

void LcdWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit timeChanged(m_currentTime);
    QWidget::mousePressEvent(event);
}

void LcdWidget::onAnimationTick()
{
    if (!m_playing) return;

    qreal elapsed = m_animTimer.elapsed() / 1000.0;
    m_animTimer.restart();

    qreal range = m_animTo - m_animFrom;
    qreal dt = elapsed * m_speed * range;
    m_currentTime += dt;
    if (m_currentTime >= m_animTo) {
        m_currentTime = m_animFrom + std::fmod(m_currentTime - m_animFrom, range);
        m_lastEventIndex = 0;
        std::fill(m_ddram, m_ddram + 80, 0);
        m_ddramAddress = 0;
        m_displayOn = true;
    }
    processEventsUpTo(m_currentTime);
    emit timeChanged(m_currentTime);
    update();
}
