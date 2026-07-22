// SPDX-License-Identifier: BUSL-1.1
#include "panels/overview/byte_overview_panel.hpp"
#include "session/debug_controller.hpp"
#include "theme/palette.hpp"
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <algorithm>
#include <vector>

namespace {
constexpr int kMargin = 4;

// Map a byte to a single display glyph. Printable ASCII shows as itself; other
// values get a compact stand-in so distinct byte patterns read at a glance.
QChar glyphFor(uint8_t v) {
    if (v >= 0x20 && v < 0x7f)
        return QChar(v);
    if (v == 0x00)
        return QChar(u'·');
    if (v == 0xff)
        return QChar(u'▓');
    return QChar(u'░');
}

QColor colorFor(uint8_t v) {
    if (v == 0x00)
        return theme::bytesText();
    if (v >= 0x20 && v < 0x7f)
        return theme::defaultText();
    return theme::address();
}
} // namespace

ByteOverviewPanel::ByteOverviewPanel(DebugController* controller, QWidget* parent)
    : QAbstractScrollArea(parent), controller_(controller) {
    QFont f("Consolas");
    f.setStyleHint(QFont::Monospace);
    f.setPixelSize(12);
    setFont(f);
    QFontMetrics fm(f);
    cellW_ = std::max(6, fm.horizontalAdvance('0'));
    cellH_ = fm.height();
    gutterW_ = fm.horizontalAdvance("0000000000") + kMargin;
    viewport()->setCursor(Qt::PointingHandCursor);
    horizontalScrollBar()->setEnabled(false);
    connect(controller_, &DebugController::stateChanged, this, &ByteOverviewPanel::refresh);
    connect(controller_, &DebugController::navigationRequested, this, &ByteOverviewPanel::setBase);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int) { viewport()->update(); });
}

void ByteOverviewPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaSection* secs = nullptr;
    size_t n = irida_sections(s, &secs);

    // Span the whole image, from the lowest section to the end of the highest.
    start_ = 0;
    end_ = 0;
    bool first = true;
    for (size_t i = 0; i < n; ++i) {
        if (secs[i].vsize == 0)
            continue;
        uint64_t lo = secs[i].vaddr;
        uint64_t hi = secs[i].vaddr + secs[i].vsize;
        if (first) {
            start_ = lo;
            end_ = hi;
            first = false;
        } else {
            start_ = std::min(start_, lo);
            end_ = std::max(end_, hi);
        }
    }
    if (first) {
        start_ = 0;
        end_ = 0;
    }
    top_ = start_;
    updateScrollRange();
    viewport()->update();
}

void ByteOverviewPanel::setBase(uint64_t addr) {
    if (end_ == 0 || addr < start_ || addr >= end_)
        return;
    int cols = columns();
    top_ = start_ + ((addr - start_) / cols) * cols;
    updateScrollRange();
    int row = static_cast<int>((top_ - start_) / cols);
    verticalScrollBar()->setValue(row);
    viewport()->update();
}

int ByteOverviewPanel::columns() const {
    int usable = viewport()->width() - gutterW_ - kMargin;
    return std::max(1, usable / cellW_);
}

int ByteOverviewPanel::visibleRows() const {
    return std::max(1, (viewport()->height() - kMargin) / cellH_);
}

void ByteOverviewPanel::updateScrollRange() {
    if (end_ <= start_) {
        verticalScrollBar()->setRange(0, 0);
        return;
    }
    int cols = columns();
    uint64_t bytes = end_ - start_;
    int totalRows = static_cast<int>((bytes + cols - 1) / cols);
    verticalScrollBar()->setRange(0, std::max(0, totalRows - visibleRows()));
    verticalScrollBar()->setPageStep(visibleRows());
}

void ByteOverviewPanel::resizeEvent(QResizeEvent*) {
    updateScrollRange();
    viewport()->update();
}

uint64_t ByteOverviewPanel::addressAt(const QPoint& pos) const {
    if (end_ <= start_)
        return 0;
    int cols = columns();
    int row = verticalScrollBar()->value() + (pos.y() - kMargin) / cellH_;
    int col = (pos.x() - gutterW_) / cellW_;
    col = std::clamp(col, 0, cols - 1);
    if (row < 0)
        return 0;
    uint64_t addr = start_ + static_cast<uint64_t>(row) * cols + col;
    return (addr < end_) ? addr : 0;
}

void ByteOverviewPanel::mousePressEvent(QMouseEvent* event) {
    uint64_t addr = addressAt(event->pos());
    if (addr != 0)
        controller_->navigateTo(addr);
}

void ByteOverviewPanel::paintEvent(QPaintEvent*) {
    QPainter p(viewport());
    p.fillRect(viewport()->rect(), theme::background());
    if (end_ <= start_)
        return;

    p.setFont(font());
    int cols = columns();
    int rows = visibleRows();
    int topRow = verticalScrollBar()->value();

    std::vector<uint8_t> buf(static_cast<size_t>(cols));
    IridaSession* s = controller_->session();

    for (int r = 0; r <= rows; ++r) {
        uint64_t rowAddr = start_ + static_cast<uint64_t>(topRow + r) * cols;
        if (rowAddr >= end_)
            break;
        int y = kMargin + r * cellH_;

        p.setPen(theme::address());
        p.drawText(kMargin, y + cellH_ - 3, QString("%1").arg(rowAddr, 10, 16, QChar('0')));

        size_t want = static_cast<size_t>(std::min<uint64_t>(cols, end_ - rowAddr));
        size_t got = irida_read_memory(s, rowAddr, buf.data(), want);
        for (size_t c = 0; c < want; ++c) {
            int x = gutterW_ + static_cast<int>(c) * cellW_;
            if (c < got) {
                p.setPen(colorFor(buf[c]));
                p.drawText(x, y + cellH_ - 3, QString(glyphFor(buf[c])));
            } else {
                p.setPen(theme::bytesText());
                p.drawText(x, y + cellH_ - 3, QStringLiteral("?"));
            }
        }
    }
}
