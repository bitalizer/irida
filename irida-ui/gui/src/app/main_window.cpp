// SPDX-License-Identifier: BUSL-1.1
#include "app/main_window.hpp"
#include "layouts/cpu_widget.hpp"
#include "panels/execution/breakpoints_panel.hpp"
#include "panels/memory/memory_panel.hpp"
#include "panels/symbols/modules_panel.hpp"
#include "session/debug_controller.hpp"
#include "theme/icons.hpp"
#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QSettings>
#include <QToolBar>

MainWindow::MainWindow(IridaSession* session, QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Irida");
    controller_ = new DebugController(session, this);
    cpu_ = new CpuWidget(controller_, this);
    setCentralWidget(cpu_);

    buildToolbar();
    buildDocks();

    // navigation: follow address in memory panel + scroll disasm
    connect(controller_, &DebugController::navigationRequested, this, [this](uint64_t addr) {
        cpu_->memory()->setBase(addr);
        cpu_->memory()->refresh();
    });

    resize(1200, 800);
    restoreLayout();
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
}

void MainWindow::buildDocks() {
    modules_ = new ModulesPanel(controller_, this);
    breakpoints_ = new BreakpointsPanel(controller_, this);

    auto* modDock = new QDockWidget("Modules", this);
    modDock->setObjectName("ModulesDock");
    modDock->setWidget(modules_);
    addDockWidget(Qt::RightDockWidgetArea, modDock);

    auto* bpDock = new QDockWidget("Breakpoints", this);
    bpDock->setObjectName("BreakpointsDock");
    bpDock->setWidget(breakpoints_);
    addDockWidget(Qt::RightDockWidgetArea, bpDock);

    tabifyDockWidget(modDock, bpDock);
    modDock->raise();
}

void MainWindow::restoreLayout() {
    QSettings s("Irida", "Irida");
    restoreGeometry(s.value("geometry").toByteArray());
    restoreState(s.value("windowState").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings s("Irida", "Irida");
    s.setValue("geometry", saveGeometry());
    s.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}
