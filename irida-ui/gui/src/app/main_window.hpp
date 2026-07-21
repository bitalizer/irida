// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/irida.h"
#include <QMainWindow>
class DebugController;
class CpuWidget;
class ModulesPanel;
class BreakpointsPanel;
class ThreadsPanel;
class MemoryMapPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(IridaSession* session, QWidget* parent = nullptr);

  protected:
    void closeEvent(QCloseEvent* event) override;

  private:
    void buildMenuBar();
    void buildToolbar();
    void buildDocks();
    void restoreLayout();
    void attachToProcess();

    DebugController* controller_;
    CpuWidget* cpu_;
    ModulesPanel* modules_;
    BreakpointsPanel* breakpoints_;
    ThreadsPanel* threads_;
    MemoryMapPanel* memoryMap_;
};
