// SPDX-License-Identifier: BUSL-1.1
#include "panels/graph/graph_view.hpp"
#include "session/debug_controller.hpp"
#include "theme/palette.hpp"
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QPen>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <unordered_map>

namespace {

constexpr double kBlockPaddingX = 10.0;
constexpr double kBlockPaddingY = 8.0;
constexpr double kHorizontalSpacing = 40.0;
constexpr double kVerticalSpacing = 60.0;
constexpr double kArrowSize = 8.0;
constexpr size_t kMaxBlockInsns = 256; // upper bound on instructions decoded per block

// A basic-block box that reports double-clicks back to the GraphView so it can
// navigate the session. Carries its block address as identity.
class BlockItem : public QGraphicsRectItem {
  public:
    BlockItem(uint64_t addr, std::function<void(uint64_t)> onActivate)
        : addr_(addr), on_activate_(std::move(onActivate)) {}

  protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && on_activate_)
            on_activate_(addr_);
        QGraphicsRectItem::mouseDoubleClickEvent(event);
    }

  private:
    uint64_t addr_;
    std::function<void(uint64_t)> on_activate_;
};

// Appends an arrowhead to path pointing from `from` into `to`.
void addArrowHead(QPainterPath& path, const QPointF& from, const QPointF& to) {
    QLineF line(from, to);
    double len = line.length();
    if (len < 1e-6)
        return;
    QPointF dir((to.x() - from.x()) / len, (to.y() - from.y()) / len);
    QPointF normal(-dir.y(), dir.x());
    QPointF base = to - dir * kArrowSize;
    QPointF left = base + normal * (kArrowSize * 0.5);
    QPointF right = base - normal * (kArrowSize * 0.5);
    path.moveTo(to);
    path.lineTo(left);
    path.lineTo(right);
    path.lineTo(to);
}

} // namespace

GraphView::GraphView(DebugController* controller, QWidget* parent)
    : QGraphicsView(parent), controller_(controller) {
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setBackgroundBrush(theme::graphBackground());
    setRenderHint(QPainter::Antialiasing, true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    font_ = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font_.setPointSize(9);

    connect(controller_, &DebugController::stateChanged, this, &GraphView::refresh);
    connect(controller_, &DebugController::navigationRequested, this, &GraphView::onNavigation);
}

void GraphView::refresh() {
    IridaSession* s = controller_->session();
    irida_analyze(s);
    if (current_fn_ == 0) {
        const IridaFunction* fns = nullptr;
        size_t n = irida_functions(s, &fns);
        if (n > 0)
            current_fn_ = fns[0].addr;
    }
    if (current_fn_ != 0)
        showFunction(current_fn_);
}

void GraphView::onNavigation(uint64_t addr) {
    IridaSession* s = controller_->session();
    irida_analyze(s);
    const IridaFunction* fns = nullptr;
    size_t n = irida_functions(s, &fns);
    for (size_t i = 0; i < n; ++i) {
        if (addr >= fns[i].addr && addr < fns[i].addr + fns[i].size) {
            showFunction(fns[i].addr);
            return;
        }
    }
}

std::vector<GraphView::Block> GraphView::buildBlocks(uint64_t fn_addr) const {
    IridaSession* s = controller_->session();
    const IridaBasicBlock* bbs = nullptr;
    size_t n = irida_function_blocks(s, fn_addr, &bbs);

    std::vector<Block> blocks;
    blocks.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        Block b;
        b.bb = bbs[i];
        // Decode a generous run from the block start in one call, then keep the
        // instructions that fall inside [addr, addr+size). One disasm call per
        // block keeps the borrowed row array valid for the whole loop.
        const IridaInsnRow* rows = nullptr;
        size_t got = irida_disasm(s, b.bb.addr, kMaxBlockInsns, &rows);
        uint64_t end = b.bb.addr + b.bb.size;
        for (size_t r = 0; r < got; ++r) {
            if (rows[r].address >= end)
                break;
            QString addr = QString("%1").arg(rows[r].address, 0, 16);
            QString text = QString::fromUtf8(rows[r].text ? rows[r].text : "");
            b.lines << (addr + "  " + text);
        }
        if (b.lines.isEmpty())
            b.lines << QString("%1").arg(b.bb.addr, 0, 16);
        blocks.push_back(std::move(b));
    }
    return blocks;
}

void GraphView::layout(std::vector<Block>& blocks) const {
    if (blocks.empty())
        return;

    std::unordered_map<uint64_t, size_t> index;
    for (size_t i = 0; i < blocks.size(); ++i)
        index[blocks[i].bb.addr] = i;

    // Size each block from its text extent in the monospace font.
    QFontMetricsF fm(font_);
    double lineHeight = fm.height();
    for (Block& b : blocks) {
        double maxW = 0.0;
        for (const QString& line : b.lines)
            maxW = std::max(maxW, fm.horizontalAdvance(line));
        b.width = maxW + 2 * kBlockPaddingX;
        b.height = lineHeight * b.lines.size() + 2 * kBlockPaddingY;
    }

    // Layer assignment: BFS from the entry block (the first, lowest address)
    // over successor edges, layer = first-seen depth. Back-edges into an
    // already-visited block are ignored so loops don't recurse forever.
    std::vector<int> layer(blocks.size(), -1);
    std::vector<size_t> order;
    layer[0] = 0;
    order.push_back(0);
    for (size_t head = 0; head < order.size(); ++head) {
        size_t cur = order[head];
        int nextLayer = layer[cur] + 1;
        const uint64_t succ[2] = {blocks[cur].bb.jump, blocks[cur].bb.fail};
        for (uint64_t target : succ) {
            if (target == 0)
                continue;
            auto it = index.find(target);
            if (it == index.end())
                continue;
            size_t ti = it->second;
            if (layer[ti] == -1) {
                layer[ti] = nextLayer;
                order.push_back(ti);
            }
        }
    }
    // Blocks unreachable from entry (shouldn't happen for a well-formed CFG)
    // still get placed below everything so they remain visible.
    int maxLayer = 0;
    for (int l : layer)
        maxLayer = std::max(maxLayer, l);
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (layer[i] == -1)
            layer[i] = ++maxLayer;
        blocks[i].layer = layer[i];
    }

    // Column assignment within each layer: stable order by address.
    std::unordered_map<int, std::vector<size_t>> byLayer;
    for (size_t i = 0; i < blocks.size(); ++i)
        byLayer[blocks[i].layer].push_back(i);
    for (auto& [lvl, ids] : byLayer) {
        std::sort(ids.begin(), ids.end(),
                  [&](size_t a, size_t b) { return blocks[a].bb.addr < blocks[b].bb.addr; });
        for (size_t c = 0; c < ids.size(); ++c)
            blocks[ids[c]].column = static_cast<int>(c);
    }

    // Pixel positions. Each layer's row height is its tallest block; each
    // layer is centered horizontally around x = 0.
    int lastLayer = 0;
    for (const Block& b : blocks)
        lastLayer = std::max(lastLayer, b.layer);

    std::vector<double> layerY(lastLayer + 1, 0.0);
    double y = 0.0;
    for (int l = 0; l <= lastLayer; ++l) {
        layerY[l] = y;
        double rowHeight = 0.0;
        for (const Block& b : blocks)
            if (b.layer == l)
                rowHeight = std::max(rowHeight, b.height);
        y += rowHeight + kVerticalSpacing;
    }

    for (auto& [lvl, ids] : byLayer) {
        double totalWidth = 0.0;
        for (size_t id : ids)
            totalWidth += blocks[id].width;
        totalWidth += kHorizontalSpacing * (ids.size() > 0 ? ids.size() - 1 : 0);
        double x = -totalWidth / 2.0;
        for (size_t id : ids) {
            blocks[id].x = x;
            blocks[id].y = layerY[lvl];
            x += blocks[id].width + kHorizontalSpacing;
        }
    }
}

void GraphView::draw(const std::vector<Block>& blocks) {
    scene_->clear();
    if (blocks.empty())
        return;

    std::unordered_map<uint64_t, size_t> index;
    for (size_t i = 0; i < blocks.size(); ++i)
        index[blocks[i].bb.addr] = i;

    QFontMetricsF fm(font_);
    double lineHeight = fm.height();

    // Edges first so blocks paint on top of the arrows' endpoints.
    auto drawEdge = [&](const Block& from, const Block& to, const QColor& color) {
        QPointF start(from.x + from.width / 2.0, from.y + from.height);
        QPointF end(to.x + to.width / 2.0, to.y);
        QPainterPath path;
        path.moveTo(start);
        // Orthogonal routing: down out of the source, across at the midpoint,
        // then down into the target.
        double midY = (start.y() + end.y()) / 2.0;
        path.lineTo(QPointF(start.x(), midY));
        path.lineTo(QPointF(end.x(), midY));
        path.lineTo(end);
        auto* line = scene_->addPath(path, QPen(color, 1.5));
        line->setZValue(-1.0);
        QPainterPath head;
        addArrowHead(head, QPointF(end.x(), midY), end);
        auto* arrow = scene_->addPath(head, QPen(color, 1.5), QBrush(color));
        arrow->setZValue(-1.0);
    };

    for (const Block& b : blocks) {
        bool conditional = b.bb.jump != 0 && b.bb.fail != 0;
        auto edgeTo = [&](uint64_t target, const QColor& color) {
            if (target == 0)
                return;
            auto it = index.find(target);
            if (it == index.end())
                return;
            drawEdge(b, blocks[it->second], color);
        };
        if (conditional) {
            edgeTo(b.bb.jump, theme::graphEdgeTaken());
            edgeTo(b.bb.fail, theme::graphEdgeFail());
        } else if (b.bb.jump != 0) {
            edgeTo(b.bb.jump, theme::graphEdgeUncond());
        } else if (b.bb.fail != 0) {
            edgeTo(b.bb.fail, theme::graphEdgeUncond());
        }
    }

    for (size_t i = 0; i < blocks.size(); ++i) {
        const Block& b = blocks[i];
        auto* rect = new BlockItem(b.bb.addr, [this](uint64_t addr) { onBlockActivated(addr); });
        rect->setRect(b.x, b.y, b.width, b.height);
        rect->setBrush(theme::graphBlockBackground());
        bool isEntry = (i == 0);
        rect->setPen(QPen(isEntry ? theme::graphEntryBorder() : theme::graphBlockBorder(),
                          isEntry ? 2.0 : 1.0));
        rect->setZValue(0.0);
        scene_->addItem(rect);

        double ty = b.y + kBlockPaddingY;
        for (const QString& line : b.lines) {
            auto* text = scene_->addSimpleText(line, font_);
            text->setBrush(theme::defaultText());
            text->setPos(b.x + kBlockPaddingX, ty);
            text->setParentItem(rect);
            ty += lineHeight;
        }
    }

    QRectF bounds = scene_->itemsBoundingRect();
    scene_->setSceneRect(bounds.adjusted(-40, -40, 40, 40));
}

void GraphView::showFunction(uint64_t fn_addr) {
    current_fn_ = fn_addr;
    std::vector<Block> blocks = buildBlocks(fn_addr);
    layout(blocks);
    draw(blocks);
}

void GraphView::onBlockActivated(uint64_t addr) {
    controller_->navigateTo(addr);
}

void GraphView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        scale(factor, factor);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}
