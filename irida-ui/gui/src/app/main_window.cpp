// SPDX-License-Identifier: BUSL-1.1
#include "app/main_window.hpp"
#include "dialogs/attach_proc_dialog.hpp"
#include "layouts/cpu_widget.hpp"
#include "panels/analysis/functions_panel.hpp"
#include "panels/analysis/xrefs_panel.hpp"
#include "panels/binfmt/exports_panel.hpp"
#include "panels/binfmt/imports_panel.hpp"
#include "panels/binfmt/sections_panel.hpp"
#include "panels/binfmt/strings_panel.hpp"
#include "panels/binfmt/symbols_panel.hpp"
#include "panels/console/console_panel.hpp"
#include "panels/execution/backtrace_panel.hpp"
#include "panels/execution/breakpoints_panel.hpp"
#include "panels/execution/threads_panel.hpp"
#include "panels/memory/memory_map_panel.hpp"
#include "panels/overview/overview_bar.hpp"
#include "panels/symbols/modules_panel.hpp"
#include "session/debug_controller.hpp"
#include "theme/icons.hpp"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QScreen>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

namespace {
// Bump when the fixed layout (toolbars, dock defaults) changes so a window
// state saved by an older build is discarded instead of restored on top of the
// new layout — otherwise a stale blob can, for example, pull the overview bar
// back up next to the toolbar buttons.
constexpr int kLayoutStateVersion = 11;
} // namespace

MainWindow::MainWindow(IridaSession* session, SessionKind kind, bool autoAnalyze, QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle("Irida");
    controller_ = new DebugController(session, this, kind);
    cpu_ = new CpuWidget(controller_, this);
    setCentralWidget(cpu_);

    buildMenuBar();
    buildToolbar();
    buildDocks();
    buildViewMenu();
    buildStatusBar();

    // Snapshot the freshly-built layout (with every dock present) so Reset
    // Layout and the attach transition can return to the full arrangement.
    defaultState_ = saveState(kLayoutStateVersion);

    resize(1200, 800);
    restoreLayout();

    // Debug docks track the session kind: hidden for a static file, shown once a
    // live process is attached.
    applyDebugDockVisibility();
    connect(controller_, &DebugController::sessionKindChanged, this,
            &MainWindow::applyDebugDockVisibility);

    // Populate the panels after construction returns and the window is shown,
    // so the first data load runs on the event loop rather than blocking the
    // window from ever appearing, then kick off analysis on a worker thread.
    QTimer::singleShot(0, controller_, [this, autoAnalyze] {
        controller_->refreshViews();
        if (autoAnalyze)
            controller_->runAnalysis();
    });
}

void MainWindow::applyDebugDockVisibility() {
    bool live = controller_->isLive();
    for (QDockWidget* dock : debugDocks_)
        dock->setVisible(live);
}

void MainWindow::buildMenuBar() {
    auto* fileMenu = menuBar()->addMenu("&File");

    QAction* attachAction = fileMenu->addAction("&Attach to Process...");
    connect(attachAction, &QAction::triggered, this, &MainWindow::attachToProcess);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
}

void MainWindow::attachToProcess() {
    AttachProcDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    uint32_t pid = dlg.selectedPid();
    if (pid == 0)
        return;

    IridaSession* newSession = irida_session_create_native(pid);
    if (!newSession) {
        QMessageBox::warning(this, "Attach failed",
                             QString("Failed to attach to process %1. It may be protected or "
                                     "require elevation.")
                                 .arg(pid));
        return;
    }

    controller_->setSession(newSession, SessionKind::Native);
}

void MainWindow::buildToolbar() {
    auto* tb = addToolBar("Debug");
    tb->setObjectName("DebugToolbar");
    struct A {
        const char* icon;
        const char* text;
        void (DebugController::*slot)();
    };
    const A actions[] = {
        {"play", "Run", &DebugController::run},
        {"pause", "Break", &DebugController::breakExec},
        {"arrow-down-to-line", "Step Into", &DebugController::stepInto},
        {"redo", "Step Over", &DebugController::stepOver},
        {"arrow-up-from-line", "Step Out", &DebugController::stepOut},
        {"rotate-ccw", "Restart", &DebugController::restart},
        {"square", "Stop", &DebugController::stop},
    };
    for (const auto& a : actions) {
        QAction* act = tb->addAction(icons::load(a.icon), a.text);
        connect(act, &QAction::triggered, controller_, a.slot);
    }

    // The overview bar sits on its own full-width toolbar row below the debug
    // controls, so it spans the whole window.
    addToolBarBreak(Qt::TopToolBarArea);
    auto* overviewBar = new QToolBar("Overview", this);
    overviewBar->setObjectName("OverviewToolbar");
    overviewBar->setMovable(false);
    overviewBar->setFloatable(false);
    addToolBar(Qt::TopToolBarArea, overviewBar);
    overview_ = new OverviewBar(controller_, this);
    overview_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    overviewBar->addWidget(overview_);
}

void MainWindow::buildDocks() {
    modules_ = new ModulesPanel(controller_, this);
    breakpoints_ = new BreakpointsPanel(controller_, this);
    threads_ = new ThreadsPanel(controller_, this);
    memoryMap_ = new MemoryMapPanel(controller_, this);
    backtrace_ = new BacktracePanel(controller_, this);
    console_ = new ConsolePanel(controller_, this);
    sections_ = new SectionsPanel(controller_, this);
    imports_ = new ImportsPanel(controller_, this);
    exports_ = new ExportsPanel(controller_, this);
    symbols_ = new SymbolsPanel(controller_, this);
    strings_ = new StringsPanel(controller_, this);
    functions_ = new FunctionsPanel(controller_, this);
    xrefs_ = new XrefsPanel(controller_, this);

    // Creates a dock, tracks it for the View menu, and adds it to an area.
    auto addDock = [this](const char* title, const char* objectName, QWidget* widget,
                          Qt::DockWidgetArea area) {
        auto* dock = new QDockWidget(title, this);
        dock->setObjectName(objectName);
        dock->setWidget(widget);
        addDockWidget(area, dock);
        docks_.push_back(dock);
        return dock;
    };

    // The right-side group is debug-only: it means nothing without a live
    // process, so it is tracked separately and shown only when one is attached.
    auto* modDock = addDock("Modules", "ModulesDock", modules_, Qt::RightDockWidgetArea);
    auto* bpDock = addDock("Breakpoints", "BreakpointsDock", breakpoints_, Qt::RightDockWidgetArea);
    auto* threadsDock = addDock("Threads", "ThreadsDock", threads_, Qt::RightDockWidgetArea);
    auto* mapDock = addDock("Memory Map", "MemoryMapDock", memoryMap_, Qt::RightDockWidgetArea);
    auto* btDock = addDock("Backtrace", "BacktraceDock", backtrace_, Qt::RightDockWidgetArea);
    debugDocks_ = {modDock, bpDock, threadsDock, mapDock, btDock};

    tabifyDockWidget(modDock, bpDock);
    tabifyDockWidget(bpDock, threadsDock);
    tabifyDockWidget(threadsDock, mapDock);
    tabifyDockWidget(mapDock, btDock);
    modDock->raise();

    // Console spans the full width along the very bottom, in its own area.
    addDock("Console", "ConsoleDock", console_, Qt::BottomDockWidgetArea);

    // The left column is a single dock holding a fixed two-row split: the binary
    // tabs on top and Cross References beneath. Keeping them in one dock (joined
    // by a splitter) guarantees Xrefs stays its own row and never collapses into
    // the tab bar, which is what happens when they are separate docks in the
    // same area.
    auto* binfmtTabs = new QTabWidget(this);
    binfmtTabs->setDocumentMode(true);
    // Functions leads: it's the primary way into a binary, so it's the first tab.
    binfmtTabs->addTab(functions_, "Functions");
    binfmtTabs->addTab(sections_, "Sections");
    binfmtTabs->addTab(imports_, "Imports");
    binfmtTabs->addTab(exports_, "Exports");
    binfmtTabs->addTab(symbols_, "Symbols");
    binfmtTabs->addTab(strings_, "Strings");

    auto* xrefsHeader = new QLabel("Cross References", this);
    xrefsHeader->setContentsMargins(6, 4, 6, 4);
    auto* xrefsPane = new QWidget(this);
    auto* xrefsLayout = new QVBoxLayout(xrefsPane);
    xrefsLayout->setContentsMargins(0, 0, 0, 0);
    xrefsLayout->setSpacing(0);
    xrefsLayout->addWidget(xrefsHeader);
    xrefsLayout->addWidget(xrefs_);

    auto* leftSplit = new QSplitter(Qt::Vertical, this);
    leftSplit->addWidget(binfmtTabs);
    leftSplit->addWidget(xrefsPane);
    leftSplit->setStretchFactor(0, 3);
    leftSplit->setStretchFactor(1, 2);

    auto* leftDock = new QDockWidget("Explorer", this);
    leftDock->setObjectName("ExplorerDock");
    leftDock->setWidget(leftSplit);
    addDockWidget(Qt::LeftDockWidgetArea, leftDock);
    docks_.push_back(leftDock);
}

void MainWindow::buildViewMenu() {
    auto* viewMenu = menuBar()->addMenu("&View");
    // One checkable toggle per dock — closing a dock (its X) unchecks the entry,
    // and re-checking it here brings the panel back, so nothing is ever lost.
    for (QDockWidget* dock : docks_)
        viewMenu->addAction(dock->toggleViewAction());
    viewMenu->addSeparator();
    QAction* reset = viewMenu->addAction("Reset Layout");
    connect(reset, &QAction::triggered, this, &MainWindow::resetLayout);
}

void MainWindow::resetLayout() {
    for (QDockWidget* dock : docks_)
        dock->setVisible(true);
    restoreState(defaultState_, kLayoutStateVersion);
    // The snapshot was taken with every dock present; re-apply the mode rule so
    // Reset Layout does not resurrect the debug-only docks in static mode.
    applyDebugDockVisibility();
}

void MainWindow::buildStatusBar() {
    statusLabel_ = new QLabel("Ready", this);
    statusBar()->addWidget(statusLabel_);

    // An indeterminate bar (range 0..0) shown only while analysis runs.
    statusProgress_ = new QProgressBar(this);
    statusProgress_->setRange(0, 0);
    statusProgress_->setMaximumWidth(160);
    statusProgress_->setVisible(false);
    statusBar()->addPermanentWidget(statusProgress_);

    connect(controller_, &DebugController::analysisStarted, this, [this] {
        statusLabel_->setText("Analyzing…");
        statusProgress_->setVisible(true);
    });
    connect(controller_, &DebugController::analysisFinished, this, [this] {
        const IridaFunction* fns = nullptr;
        size_t n = irida_functions(controller_->session(), &fns);
        statusLabel_->setText(QString("Analysis complete — %1 functions").arg(n));
        statusProgress_->setVisible(false);
    });
}

void MainWindow::restoreLayout() {
    QSettings s("Irida", "Irida");
    QByteArray geometry = s.value("geometry").toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);

    // A geometry saved on a monitor that is no longer present (unplugged,
    // resolution changed) can place the window entirely off every screen,
    // leaving it invisible. If the restored frame does not intersect any
    // available screen, drop back to a centered default.
    bool onScreen = false;
    for (const QScreen* screen : QApplication::screens()) {
        if (screen->availableGeometry().intersects(frameGeometry())) {
            onScreen = true;
            break;
        }
    }
    if (!onScreen) {
        resize(1200, 800);
        if (QScreen* primary = QApplication::primaryScreen())
            move(primary->availableGeometry().center() - rect().center());
    }

    // Versioned restore: a state blob written by an older build (different
    // toolbar/dock layout) fails the version check and is ignored, so the
    // freshly-built layout stands instead of being overwritten by a stale one.
    restoreState(s.value("windowState").toByteArray(), kLayoutStateVersion);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings s("Irida", "Irida");
    s.setValue("geometry", saveGeometry());
    s.setValue("windowState", saveState(kLayoutStateVersion));
    QMainWindow::closeEvent(event);
}
