// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QWidget>
#include <cstdint>
class DebugController;
class DisassemblyPanel;
class GraphView;
class MemoryPanel;
class ByteOverviewPanel;
class QStackedWidget;
class QToolButton;
class QMenu;

// The main code view. A header bar selects, in three stages, how the binary is
// shown: the data source (PE / RAW), the view form (Bytes / Hex / Graph /
// Linear), and — for Graph and Linear only — the representation (Disassembly /
// Low Level IL / Pseudo C). The chosen combination swaps the widget below. IL
// and Pseudo C representations are present but disabled until their engines
// exist; Pseudo C unlocks with the decompiler.
class DisassemblyView : public QWidget {
    Q_OBJECT
  public:
    explicit DisassemblyView(DebugController* controller, QWidget* parent = nullptr);

    DisassemblyPanel* disassembly() const {
        return linear_;
    }

  private:
    enum class View { Bytes, Hex, Graph, Linear };

    void buildHeader();
    void selectView(View v);
    void updateReprVisibility();

    DebugController* controller_;
    QStackedWidget* stack_;
    DisassemblyPanel* linear_;
    GraphView* graph_;
    MemoryPanel* hex_;
    ByteOverviewPanel* bytes_;

    QToolButton* sourceButton_;
    QToolButton* viewButton_;
    QToolButton* reprButton_;

    View view_ = View::Linear;
};
