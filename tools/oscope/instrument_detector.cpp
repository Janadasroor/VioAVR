#include "instrument_detector.h"
#include <QFile>
#include <QRegularExpression>
#include <QMap>
#include <QSet>
#include <algorithm>

QString pinSignalUserRole(const QString &sigName)
{
    QRegularExpression re("^v\\((.+)\\)$");
    auto m = re.match(sigName);
    if (m.hasMatch())
        return m.captured(1);
    return sigName;
}

QList<PinSignal> detectLedMatrixSignals(const QStringList &allSignalNames)
{
    QRegularExpression pinRe("^p([a-eA-E])([0-7])_an$");
    QMap<QString, QMap<int, QString>> byPort;

    for (const auto &raw : allSignalNames) {
        QString s = pinSignalUserRole(raw);
        auto m = pinRe.match(s);
        if (m.hasMatch()) {
            char port = m.captured(1).toLower().at(0).toLatin1();
            int bit = m.captured(2).toInt();
            byPort[QString(port)][bit] = s;
        }
    }

    if (byPort.isEmpty())
        return {};

    QList<PinSignal> result;
    QStringList ports = byPort.keys();
    std::sort(ports.begin(), ports.end());
    for (const auto &p : ports) {
        const auto &bits = byPort[p];
        for (auto it = bits.begin(); it != bits.end(); ++it) {
            result.append({p.toLatin1().at(0), it.key(), it.value()});
        }
    }
    return result;
}

static void tokenizeBracketNodes(const QString &raw, QStringList &out)
{
    QString work = raw;
    work.replace('[', ' ');
    work.replace(']', ' ');
    out = work.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}

QList<InstrumentDef> detectInstruments(const QString &cirPath)
{
    QList<InstrumentDef> result;
    QFile f(cirPath);
    if (!f.open(QIODevice::ReadOnly))
        return result;

    QString content = QString::fromUtf8(f.readAll());
    f.close();

    // --- Pattern: X<name> ... <any-nodes> led_matrix_10x10 ---
    {
        QRegularExpression matrixRe(
            "^X(\\w+)\\s+"            // X<name>
            "(.+)"                    // everything (greedy)
            "\\bled_matrix_10x10\\b", // subcircuit type
            QRegularExpression::MultilineOption
        );

        auto m = matrixRe.match(content);
        if (m.hasMatch()) {
            QString raw = m.captured(2).trimmed();
            QStringList nodes;
            tokenizeBracketNodes(raw, nodes);

            if (nodes.size() == 20) {
                InstrumentDef def;
                def.type = "led_matrix_10x10";
                def.name = m.captured(1);
                def.rows = 10;
                def.cols = 10;

                for (int i = 0; i < 10 && i < nodes.size(); i++)
                    def.rowSignals.append(nodes[i]);
                for (int i = 10; i < 20 && i < nodes.size(); i++)
                    def.colSignals.append(nodes[i]);

                result.append(def);
            }
        }
    }

    // --- Pattern: X<name> ... <any-nodes> 7seg_display ---
    {
        QRegularExpression segRe(
            "^X(\\w+)\\s+"
            "(.+)"
            "\\b7seg_display\\b",
            QRegularExpression::MultilineOption
        );

        auto m = segRe.match(content);
        if (m.hasMatch()) {
            QString raw = m.captured(2).trimmed();
            QStringList nodes;
            tokenizeBracketNodes(raw, nodes);

            if (nodes.size() >= 7 && (nodes.size() % 7) == 0) {
                InstrumentDef def;
                def.type = "7seg_display";
                def.name = m.captured(1);
                def.signalNames = nodes;

                result.append(def);
            }
        }
    }

    // --- Pattern: X<name> ... <any-nodes> lcd_hd44780 ---
    {
        QRegularExpression lcdRe(
            "^X(\\w+)\\s+"
            "(.+)"
            "\\blcd_hd44780\\b",
            QRegularExpression::MultilineOption
        );

        auto m = lcdRe.match(content);
        if (m.hasMatch()) {
            QString raw = m.captured(2).trimmed();
            QStringList nodes;
            tokenizeBracketNodes(raw, nodes);

            if (nodes.size() == 6) {
                InstrumentDef def;
                def.type = "lcd_hd44780";
                def.name = m.captured(1);
                // Order: RS, E, D4, D5, D6, D7
                def.signalNames = nodes;
                result.append(def);
            }
        }
    }

    return result;
}
