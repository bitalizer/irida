// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/irida.h"
#include "session/debug_controller.hpp"
#include <QMainWindow>
class DebugController;
class QLabel;
class QProgressBar;
class CpuWidget;
class ModulesPanel;
class BreakpointsPanel;
class ThreadsPanel;
class MemoryMapPanel;
class BacktracePanel;
class ConsolePanel;
class SectionsPanel;
class ImportsPanel;
class ExportsPanel;
class SymbolsPanel;
class StringsPanel;
class FunctionsPanel;
class XrefsPanel;
class GraphView;

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(IridaSession* session, SessionKind kind = SessionKind::Static,
                        QWidget* parent = nullptr);

  protected:
    void closeEvent(QCloseEvent* event) override;

  private:
    void buildMenuBar();
    void buildToolbar();
    void buildDocks();
    void buildStatusBar();
    void restoreLayout();
    void attachToProcess();

    DebugController* controller_;
    CpuWidget* cpu_;
    ModulesPanel* modules_;
    BreakpointsPanel* breakpoints_;
    ThreadsPanel* threads_;
    MemoryMapPanel* memoryMap_;
    BacktracePanel* backtrace_;
    ConsolePanel* console_;
    SectionsPanel* sections_;
    ImportsPanel* imports_;
    ExportsPanel* exports_;
    SymbolsPanel* symbols_;
    StringsPanel* strings_;
    FunctionsPanel* functions_;
    XrefsPanel* xrefs_;
    GraphView* graph_;

    QLabel* statusLabel_;
    QProgressBar* statusProgress_;
};
