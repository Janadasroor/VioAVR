#include "main_window.h"
#include "theme_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QHeaderView>
#include <QSplitter>
#include <QPushButton>
#include <QFont>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("VioAVR Oscilloscope");
    resize(1200, 700);

    QSettings settings(kOrgName, kAppName);
    m_recentFiles = settings.value(kSettingsRecent).toStringList();
    m_lastDir = settings.value(kSettingsLastDir, QDir::homePath()).toString();

    m_viewer = new WaveformViewer;
    m_viewer->setPlotQuality(WaveformViewer::PlotQuality::HighQuality);
    setCentralWidget(m_viewer);

    setupMenuBar();
    setupSidebar();

    // statusMessage — WaveformViewer doesn't have one; we emit our own
}

void MainWindow::loadLogFile(const QString &path)
{
    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this, "File Not Found", path);
        m_recentFiles.removeAll(path);
        updateRecentMenu();
        return;
    }
    m_viewer->loadCsv(path);
    m_lastDir = QFileInfo(path).absolutePath();
    addRecentFile(path);
    if (m_statusLabel)
        m_statusLabel->setText(QFileInfo(path).fileName());
}

void MainWindow::setupMenuBar()
{
    // --- File menu ---
    m_fileMenu = menuBar()->addMenu("&File");

    m_openAct = m_fileMenu->addAction("&Open Log...", QKeySequence::Open, this, &MainWindow::onOpenFile);

    m_recentMenu = m_fileMenu->addMenu("Open &Recent");
    updateRecentMenu();

    m_fileMenu->addSeparator();

    auto *openCirAct = m_fileMenu->addAction("Open &Circuit...", this, &MainWindow::onOpenCirFile);

    auto *runAct = m_fileMenu->addAction("&Run Simulation", QKeySequence("Ctrl+R"), this, &MainWindow::onRunSimulation);

    m_fileMenu->addSeparator();

    auto *quitAct = m_fileMenu->addAction("&Quit", QKeySequence::Quit, qApp, &QApplication::quit);

    // --- View menu ---
    m_viewMenu = menuBar()->addMenu("&View");

    auto *zoomFitAct = m_viewMenu->addAction("Zoom &Fit", QKeySequence("Ctrl+F"), m_viewer, &WaveformViewer::zoomFit);
    auto *zoomInAct = m_viewMenu->addAction("Zoom &In", QKeySequence::ZoomIn, m_viewer, &WaveformViewer::zoomFit);
    auto *zoomOutAct = m_viewMenu->addAction("Zoom &Out", QKeySequence("Ctrl+-"), m_viewer, &WaveformViewer::zoomFit);

    m_viewMenu->addSeparator();

    auto *toggleSidebarAct = m_viewMenu->addAction("Toggle &Sidebar", QKeySequence("Ctrl+B"));
    toggleSidebarAct->setCheckable(true);
    toggleSidebarAct->setChecked(true);
    connect(toggleSidebarAct, &QAction::toggled, this, [this](bool visible) {
        auto *dock = findChild<QDockWidget*>();
        if (dock) dock->setVisible(visible);
    });

    // --- Simulation menu ---
    m_simMenu = menuBar()->addMenu("&Simulation");
    auto *refreshAct = m_simMenu->addAction("&Refresh Circuit List", QKeySequence("F5"), this, &MainWindow::onRefreshCirList);
}

void MainWindow::setupSidebar()
{
    auto *dock = new QDockWidget("Explorer", this);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    auto *sidebar = new QWidget;
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto *cirLabel = new QLabel("Circuit Files");
    cirLabel->setStyleSheet("font-weight: bold; padding: 4px; color: #cbd5e1;");

    m_cirList = new QListWidget;
    m_cirList->setStyleSheet(
        "QListWidget { background: #1a1a2e; color: #cbd5e1; border: 1px solid #2a2a4e; }"
        "QListWidget::item { padding: 4px 8px; }"
        "QListWidget::item:hover { background: #2a2a4e; }"
        "QListWidget::item:selected { background: #3a3a5e; }");

    auto *logLabel = new QLabel("Simulation Logs");
    logLabel->setStyleSheet("font-weight: bold; padding: 4px; color: #cbd5e1;");

    m_logList = new QListWidget;
    m_logList->setStyleSheet(
        "QListWidget { background: #1a1a2e; color: #cbd5e1; border: 1px solid #2a2a4e; }"
        "QListWidget::item { padding: 4px 8px; }"
        "QListWidget::item:hover { background: #2a2a4e; }"
        "QListWidget::item:selected { background: #3a3a5e; }");

    auto *refreshBtn = new QPushButton("Refresh");
    refreshBtn->setStyleSheet(
        "QPushButton { background: #2a2a4e; color: #00f2fe; border: 1px solid #3a3a5e;"
        "  padding: 4px 12px; border-radius: 4px; }"
        "QPushButton:hover { background: #3a3a5e; }");

    layout->addWidget(cirLabel);
    layout->addWidget(m_cirList);
    layout->addWidget(logLabel);
    layout->addWidget(m_logList);
    layout->addWidget(refreshBtn);

    dock->setWidget(sidebar);
    dock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("color: #94a3b8; padding: 2px 8px; font-size: 11px;");
    statusBar()->addWidget(m_statusLabel, 1);

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 0); // indeterminate
    m_progressBar->setFixedWidth(180);
    m_progressBar->setFixedHeight(18);
    m_progressBar->setTextVisible(false);
    m_progressBar->hide();
    m_progressBar->setStyleSheet(
        "QProgressBar { background: #1a1a2e; border: 1px solid #3a3a5e; border-radius: 3px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "  stop:0 #00f2fe, stop:1 #4facfe); border-radius: 2px; }");
    statusBar()->addPermanentWidget(m_progressBar);

    connect(m_cirList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty())
            runNgspiceFor(path);
    });

    connect(m_logList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty())
            loadLogFile(path);
    });

    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshCirList);

    scanCirFiles();
}

void MainWindow::scanCirFiles()
{
    m_cirList->clear();
    m_logList->clear();

    QDir scratchDir("/home/jnd/cpp_projects/VioAVR/scratch");
    if (!scratchDir.exists()) return;

    for (const QFileInfo &info : scratchDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        // Find .cir files in each subdirectory
        QDir subDir(info.absoluteFilePath());
        for (const QFileInfo &cir : subDir.entryInfoList({"*.cir"}, QDir::Files, QDir::Name)) {
            auto *item = new QListWidgetItem(cir.fileName());
            item->setToolTip(cir.absoluteFilePath());
            item->setData(Qt::UserRole, cir.absoluteFilePath());
            item->setForeground(QColor("#7bed9f"));
            m_cirList->addItem(item);
        }

        // Find sim_matrix.log files
        QFileInfo log(subDir.absoluteFilePath("sim_matrix.log"));
        if (log.exists()) {
            auto *item = new QListWidgetItem(info.dir().dirName() + "/sim_matrix.log");
            item->setToolTip(log.absoluteFilePath());
            item->setData(Qt::UserRole, log.absoluteFilePath());
            item->setForeground(QColor("#00d2ff"));
            m_logList->addItem(item);
        }
    }

    if (m_statusLabel)
        m_statusLabel->setText(
            QString("%1 circuits, %2 logs").arg(m_cirList->count()).arg(m_logList->count()));
}

void MainWindow::onOpenFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Open Simulation Log", m_lastDir,
        "Log Files (*.log *.out *.txt);;CSV Files (*.csv);;All Files (*)");
    if (!path.isEmpty())
        loadLogFile(path);
}

void MainWindow::onOpenRecent()
{
    auto *act = qobject_cast<QAction*>(sender());
    if (act)
        loadLogFile(act->data().toString());
}

void MainWindow::onOpenCirFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Open Circuit File", m_lastDir, "Circuit Files (*.cir);;All Files (*)");
    if (!path.isEmpty())
        runNgspiceFor(path);
}

void MainWindow::onRefreshCirList()
{
    scanCirFiles();
}

void MainWindow::onRunSimulation()
{
    // If there's a selected .cir in the list, run it
    auto *item = m_cirList->currentItem();
    if (item) {
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty())
            runNgspiceFor(path);
    } else if (m_cirList->count() > 0) {
        // Run first available
        QString path = m_cirList->item(0)->data(Qt::UserRole).toString();
        if (!path.isEmpty())
            runNgspiceFor(path);
    } else {
        QMessageBox::information(this, "No Circuits", "No circuit files found in scratch/");
    }
}

void MainWindow::onClearRecent()
{
    m_recentFiles.clear();
    updateRecentMenu();
}

void MainWindow::addRecentFile(const QString &path)
{
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > kMaxRecent)
        m_recentFiles.removeLast();
    updateRecentMenu();

    QSettings settings(kOrgName, kAppName);
    settings.setValue(kSettingsRecent, m_recentFiles);
    settings.setValue(kSettingsLastDir, m_lastDir);
}

void MainWindow::updateRecentMenu()
{
    m_recentMenu->clear();
    if (m_recentFiles.isEmpty()) {
        m_recentMenu->addAction("(empty)")->setEnabled(false);
    } else {
        for (const QString &path : m_recentFiles) {
            auto *act = m_recentMenu->addAction(QFileInfo(path).fileName());
            act->setToolTip(path);
            act->setData(path);
            connect(act, &QAction::triggered, this, &MainWindow::onOpenRecent);
        }
        m_recentMenu->addSeparator();
        m_recentMenu->addAction("Clear Recent", this, &MainWindow::onClearRecent);
    }
}

static bool findBuildCosim() {
    QStringList candidates = {
        "/home/jnd/cpp_projects/VioAVR/build/cosim/libavr_cosim.so",
    };
    for (const auto &p : candidates)
        if (QFileInfo::exists(p)) return true;
    return false;
}

void MainWindow::runNgspiceFor(const QString &cirPath)
{
    QFileInfo cirInfo(cirPath);
    QString cirDir = cirInfo.absolutePath();
    QString cirName = cirInfo.fileName();

    // Detect if this circuit uses d_cosim
    bool needsCosim = false;
    {
        QFile cirFile(cirPath);
        if (cirFile.open(QIODevice::ReadOnly)) {
            needsCosim = QString(cirFile.readAll()).contains("d_cosim");
            cirFile.close();
        }
    }

    if (needsCosim) {
        // Cosim .so symlink
        QString cosimSo = cirDir + "/cosim/libavr_cosim.so";
        if (!QFileInfo::exists(cosimSo) && findBuildCosim()) {
            QDir().mkpath(cirDir + "/cosim");
            QString buildCosim = "/home/jnd/cpp_projects/VioAVR/build/cosim/libavr_cosim.so";
            QFile::link(buildCosim, cosimSo);
        }
        // Verify it was created
        if (!QFileInfo::exists(cosimSo)) {
            QMessageBox::warning(this, "Cosim Setup Failed",
                "Could not create cosim symlink.\n"
                "Build the cosim shim first:\n"
                "  cmake --build build --target avr_cosim");
            return;
        }

        // Firmware.hex symlink
        QString hexPath = cirDir + "/firmware.hex";
        if (!QFileInfo::exists(hexPath)) {
            QDir dir(cirDir);
            auto hexFiles = dir.entryInfoList({"*.hex"}, QDir::Files);
            for (const auto &fi : hexFiles) {
                if (fi.baseName() == "firmware")
                    continue;           // skip self
                if (fi.isSymLink() && !fi.exists())
                    continue;           // skip broken symlinks
                QFile::link(fi.absoluteFilePath(), hexPath);
                break;
            }
        }
    }

    QString ngspice = "/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice";
    if (!QFileInfo::exists(ngspice)) {
        QMessageBox::warning(this, "ngspice Not Found", ngspice);
        return;
    }

    m_progressBar->show();
    if (m_statusLabel)
        m_statusLabel->setText("Running: " + cirName + "...");

    auto *proc = new QProcess(this);
    proc->setWorkingDirectory(cirDir);
    proc->setProcessChannelMode(QProcess::MergedChannels);

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, cirDir, cirName](int exitCode, QProcess::ExitStatus) {
        m_progressBar->hide();
        QString logPath = cirDir + "/sim_matrix.log";
        QFile f(logPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(proc->readAllStandardOutput());
            f.close();
        }

        // Always load the log regardless of exit code — even if the AVR
        // was dead (all 0.0) the log tells the user something useful.
        loadLogFile(logPath);
        scanCirFiles();

        if (exitCode == 0) {
            if (m_statusLabel)
                m_statusLabel->setText("Done: " + cirName);
        } else {
            if (m_statusLabel)
                m_statusLabel->setText("Failed: " + cirName);
            statusBar()->showMessage(
                QString("ngspice exited with code %1 — log loaded anyway")
                    .arg(exitCode), 5000);
        }
        proc->deleteLater();
    });

    proc->start(ngspice, {"-b", cirName});
}
