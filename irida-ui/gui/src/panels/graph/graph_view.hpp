// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/irida.h"
#include <QFont>
#include <QGraphicsView>
#include <QStringList>
#include <cstdint>
#include <vector>
class DebugController;
class QGraphicsScene;

// Renders one function's basic-block control-flow graph as a node-and-edge
// diagram: each basic block is a box of disassembly, connected by arrows
// (green = conditional taken branch, red = fall-through, grey = single
// successor). Blocks are placed by a layered top-down layout built from the
// CFG's successor edges. Double-clicking a block navigates the session to it.
class GraphView : public QGraphicsView {
    Q_OBJECT
  public:
    explicit GraphView(DebugController* controller, QWidget* parent = nullptr);

  public slots:
    // Rebuilds the scene for the function at fn_addr. A no-op producing an
    // empty scene if fn_addr is not a discovered function.
    void showFunction(uint64_t fn_addr);
    // Re-runs analysis and redraws the function currently in view.
    void refresh();

  protected:
    void wheelEvent(QWheelEvent* event) override;

  private slots:
    void onNavigation(uint64_t addr);

  private:
    struct Block {
        IridaBasicBlock bb;
        QStringList lines; // one disassembly line per instruction
        int layer = 0;     // top-down depth from the entry block
        int column = 0;    // left-to-right order within the layer
        double x = 0.0;
        double y = 0.0;
        double width = 0.0;
        double height = 0.0;
    };

    std::vector<Block> buildBlocks(uint64_t fn_addr) const;
    void layout(std::vector<Block>& blocks) const;
    void draw(const std::vector<Block>& blocks);
    void onBlockActivated(uint64_t addr);

    DebugController* controller_;
    QGraphicsScene* scene_;
    uint64_t current_fn_ = 0;
    QFont font_;
};
