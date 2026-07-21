// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QWidget>
class DebugController;
class DisassemblyPanel;
class RegistersPanel;
class MemoryPanel;
class StackPanel;
class OverviewBar;

// Composes the four "hot" debugger panels into the classic CPU-view splitter
// layout: disassembly + registers on top, memory + stack on the bottom. This
// is a layout, not a domain owner — it creates the panels and arranges them;
// panel logic itself lives in panels/.
class CpuWidget : public QWidget {
    Q_OBJECT
  public:
    explicit CpuWidget(DebugController* controller, QWidget* parent = nullptr);

    DisassemblyPanel* disassembly() const {
        return disasm_;
    }
    MemoryPanel* memory() const {
        return memory_;
    }

  private:
    OverviewBar* overview_;
    DisassemblyPanel* disasm_;
    RegistersPanel* registers_;
    MemoryPanel* memory_;
    StackPanel* stack_;
};
