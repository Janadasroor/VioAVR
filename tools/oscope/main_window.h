#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QDockWidget>
#include <QLabel>
#include <QProcess>
#include <QProgressBar>

#include "waveform_viewer.h"
#include "instrument_panel.h"
#include "led_matrix_window.h"
#include "segment7_window.h"
#include "lcd_window.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    void loadLogFile(const QString &path);
    QString lastLogDir() const { return m_lastDir; }
    void setLastLogDir(const QString &dir) { m_lastDir = dir; }
    void setCurrentCirPath(const QString &path) { m_currentCirPath = path; }

private slots:
    void onOpenFile();
    void onOpenRecent();
    void onOpenCirFile();
    void onRefreshCirList();
    void onRunSimulation();
    void onClearRecent();
    void onSettings();

private:
    void setupMenuBar();
    void setupSidebar();
    void scanCirFiles();
    void addRecentFile(const QString &path);
    void updateRecentMenu();
    void runNgspiceFor(const QString &cirPath);
    void detectAndShowInstruments();

    WaveformViewer *m_viewer;
    InstrumentPanel *m_instrumentPanel;
    QListWidget *m_cirList;
    QListWidget *m_logList;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;

    QMenu *m_fileMenu;
    QMenu *m_recentMenu;
    QMenu *m_viewMenu;
    QMenu *m_simMenu;
    QAction *m_openAct;
    QAction *m_openRecentAct;
    QAction *m_clearRecentAct;

    QStringList m_recentFiles;
    QString m_lastDir;
    QString m_currentCirPath;
    QString m_lastLogPath;
    QList<LedMatrixWindow*> m_matrixWindows;
    QList<Segment7Window*> m_seg7Windows;
    QList<LcdWindow*> m_lcdWindows;
    static constexpr int kMaxRecent = 10;
    static constexpr const char *kSettingsRecent = "recentFiles";
    static constexpr const char *kSettingsLastDir = "lastDir";
    static constexpr const char *kSettingsNgspice = "ngspiceBinary";
    static constexpr const char *kDefaultNgspice = "/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice";
    static constexpr const char *kOrgName = "VioAVR";
    static constexpr const char *kAppName = "VioAVR Oscilloscope";

    static QString ngspicePath();
};
