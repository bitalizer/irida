// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QWidget>
class DebugController;
class DisassemblyView;
class DisassemblyPanel;

// Hosts the main code view. Historically this was the classic split CPU layout
// (code + registers over memory + stack); those companion panels are parked for
// now (see cpu_widget.cpp) and the code view fills the whole area.
class CpuWidget : public QWidget {
    Q_OBJECT
  public:
    explicit CpuWidget(DebugController* controller, QWidget* parent = nullptr);

    DisassemblyPanel* disassembly() const;

  private:
    DisassemblyView* codeView_;
    // RegistersPanel* registers_;
    // MemoryPanel* memory_;
    // StackPanel* stack_;
};
