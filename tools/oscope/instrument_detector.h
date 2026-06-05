#pragma once

#include <QString>
#include <QStringList>

struct InstrumentDef {
    QString type;
    QString name;
    int rows = 0;
    int cols = 0;
    QStringList signalNames;  // for pin_status: direct LED signals
    QStringList rowSignals;   // for matrix: row signal names
    QStringList colSignals;   // for matrix: column signal names
    QStringList segmentSignals; // for 7seg: [a,b,c,d,e,f,g, a,b,c,d,e,f,g, ...]
};

QList<InstrumentDef> detectInstruments(const QString &cirPath);

struct PinSignal {
    char port;
    int bit;
    QString name;
};
QList<PinSignal> detectLedMatrixSignals(const QStringList &allSignalNames);
QString pinSignalUserRole(const QString &sigName);
