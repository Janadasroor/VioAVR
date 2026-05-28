#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QProcess>
#include <QDir>
#include <QMessageBox>
#include "main_window.h"
#include "theme_manager.h"

static QString findBuildCosim() {
    QStringList candidates = {
        "/home/jnd/cpp_projects/VioAVR/build/cosim/libavr_cosim.so",
        QDir::currentPath() + "/../build/cosim/libavr_cosim.so",
    };
    for (const auto &p : candidates)
        if (QFileInfo::exists(p)) return p;
    return {};
}

static bool setupCosimLinks(const QString &cirDir) {
    QDir dir(cirDir);

    // Cosim .so symlink
    QString cosimSo = cirDir + "/cosim/libavr_cosim.so";
    if (!QFileInfo::exists(cosimSo)) {
        QString buildCosim = findBuildCosim();
        if (buildCosim.isEmpty()) return false;
        dir.mkpath("cosim");
        if (!QFile::link(buildCosim, cosimSo)) return false;
    }

    // Firmware.hex symlink
    QString hexPath = cirDir + "/firmware.hex";
    if (!QFileInfo::exists(hexPath)) {
        for (const QFileInfo &fi : dir.entryInfoList({"*.hex"}, QDir::Files)) {
            if (fi.baseName() == "firmware")
                continue;
            if (fi.isSymLink() && !fi.exists())
                continue;
            if (QFile::link(fi.absoluteFilePath(), hexPath)) break;
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("VioAVR Oscilloscope");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("VioAVR");

    ThemeManager::instance();

    QCommandLineParser parser;
    parser.setApplicationDescription("View ngspice simulation output waveforms");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("logfile", "ngspice output log (.log) file to view");

    QCommandLineOption cirOpt("cir", "Run ngspice on a .cir netlist, then open the resulting log", "path");
    parser.addOption(cirOpt);

    parser.process(app);

    auto *window = new MainWindow;

    QStringList args = parser.positionalArguments();

    // --cir flag: run ngspice first, then open the log
    if (parser.isSet(cirOpt)) {
        QString cirPath = parser.value(cirOpt);
        if (!QFileInfo::exists(cirPath)) {
            QMessageBox::critical(nullptr, "File Not Found",
                QString("Circuit file not found: %1").arg(cirPath));
            return 1;
        }

        QFileInfo cirInfo(cirPath);
        QString cirDir = cirInfo.absolutePath();
        QString cirName = cirInfo.fileName();
        QString logPath = cirDir + "/sim_matrix.log";

        // Setup cosim symlinks if needed
        bool hasCosim = false;
        {
            QFile cirFile(cirPath);
            if (cirFile.open(QIODevice::ReadOnly)) {
                hasCosim = QString(cirFile.readAll()).contains("d_cosim");
                cirFile.close();
            }
        }
        if (hasCosim)
            setupCosimLinks(cirDir);

        // Run ngspice synchronously
        QString ngspice = "/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice";
        if (!QFileInfo::exists(ngspice)) {
            QMessageBox::critical(nullptr, "ngspice Not Found",
                QString("ngspice binary not found at %1").arg(ngspice));
            return 1;
        }

        QProcess proc;
        proc.setWorkingDirectory(cirDir);
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start(ngspice, {"-b", cirName});
        if (!proc.waitForFinished(30000)) {
            proc.kill();
            QMessageBox::warning(nullptr, "Simulation Timed Out",
                "ngspice did not finish within 30 seconds.");
            return 1;
        }

        QFile f(logPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(proc.readAllStandardOutput());
            f.close();
        }

        if (proc.exitCode() != 0) {
            QMessageBox::warning(nullptr, "Simulation Failed",
                QString("ngspice exited with code %1").arg(proc.exitCode()));
        }

        if (QFileInfo::exists(logPath))
            window->loadLogFile(logPath);
    }

    // Positional arg: directly open a log file
    if (!args.isEmpty()) {
        QString path = args[0];
        if (QFileInfo::exists(path))
            window->loadLogFile(path);
    }

    // If no args and no --cir, show window with empty waveform
    window->show();
    return app.exec();
}
