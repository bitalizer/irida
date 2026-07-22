// SPDX-License-Identifier: BUSL-1.1
#include "panels/overview/byte_map_strip.hpp"
#include "session/debug_controller.hpp"
#include "theme/palette.hpp"
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace {
// Which section (by index) contains an address, or -1. Used to tint a run with
// the section palette.
int sectionIndexFor(IridaSession* s, uint64_t addr) {
    const IridaSection* secs = nullptr;
    size_t n = irida_sections(s, &secs);
    for (size_t i = 0; i < n; ++i)
        if (addr >= secs[i].vaddr && addr < secs[i].vaddr + secs[i].vsize)
            return static_cast<int>(i);
    return -1;
}

// Fraction of bytes in a run that are zero — the "black dots" in padded regions.
double zeroFraction(const uint8_t* data, int len) {
    if (len <= 0)
        return 0.0;
    int zeros = 0;
    for (int i = 0; i < len; ++i)
        if (data[i] == 0)
            ++zeros;
    return static_cast<double>(zeros) / len;
}

// Shannon entropy of a byte run, normalized to 0..1 (0 = uniform, 1 = maximal
// variety). Marks packed / compressed / encrypted regions.
double normalizedEntropy(const uint8_t* data, int len) {
    if (len <= 1)
        return 0.0;
    std::array<int, 256> hist{};
    for (int i = 0; i < len; ++i)
        ++hist[data[i]];
    double h = 0.0;
    for (int c : hist) {
        if (c == 0)
            continue;
        double p = static_cast<double>(c) / len;
        h -= p * std::log2(p);
    }
    return h / 8.0; // max entropy for bytes is 8 bits
}

QColor lerp(const QColor& a, const QColor& b, double t) {
    t = std::clamp(t, 0.0, 1.0);
    return QColor(static_cast<int>(a.red() + (b.red() - a.red()) * t),
                  static_cast<int>(a.green() + (b.green() - a.green()) * t),
                  static_cast<int>(a.blue() + (b.blue() - a.blue()) * t));
}
} // namespace

ByteMapStrip::ByteMapStrip(DebugController* controller, Mode mode, QWidget* parent)
    : QWidget(parent), controller_(controller), mode_(mode) {
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void ByteMapStrip::setRange(uint64_t start, uint64_t end) {
    start_ = start;
    end_ = end;
    rebuildCache();
    update();
}

void ByteMapStrip::refresh() {
    rebuildCache();
    update();
}

void ByteMapStrip::resizeEvent(QResizeEvent*) {
    rebuildCache();
}

int ByteMapStrip::bytesPerRow() const {
    if (end_ <= start_ || height() <= 0)
        return cols_;
    uint64_t total = end_ - start_;
    // One pixel row per screen row; spread the whole image across the height.
    int rows = std::max(1, height());
    uint64_t perRow = (total + rows - 1) / rows;
    // Round up to a multiple of the column count so each column covers an equal
    // sub-run.
    perRow = ((perRow + cols_ - 1) / cols_) * cols_;
    return static_cast<int>(std::max<uint64_t>(perRow, static_cast<uint64_t>(cols_)));
}

QColor ByteMapStrip::colorForRun(const uint8_t* bytes, int len, uint64_t addr) const {
    switch (mode_) {
    case Mode::SectionTexture: {
        int idx = sectionIndexFor(controller_->session(), addr);
        QColor base = idx >= 0 ? theme::overviewSection(idx) : theme::background();
        // Darken toward black by how much of the run is zero — padding and null
        // tables read as dark speckle over the section's base hue.
        double z = zeroFraction(bytes, len);
        return lerp(base, QColor(0x12, 0x14, 0x18), z * 0.85);
    }
    case Mode::Content: {
        // Classify the run by its dominant byte character.
        int zero = 0, ascii = 0, high = 0;
        for (int i = 0; i < len; ++i) {
            uint8_t v = bytes[i];
            if (v == 0)
                ++zero;
            else if (v >= 0x20 && v < 0x7f)
                ++ascii;
            else
                ++high;
        }
        if (len > 0 && zero >= len * 0.75)
            return QColor(0x2a, 0x2d, 0x33); // gray: padding / zeros
        if (ascii >= high && ascii > 0)
            return QColor(0x5f, 0xa8, 0x6b); // green: text
        if (high > ascii * 2)
            return QColor(0xcc, 0x82, 0x50); // orange: dense / pointers
        return QColor(0x54, 0x8c, 0xc8);     // blue: code-like
    }
    case Mode::Entropy: {
        double e = normalizedEntropy(bytes, len);
        // Cool (low) -> hot (high): dark blue -> yellow -> red.
        static const QColor cold(0x1e, 0x30, 0x50);
        static const QColor mid(0xd0, 0xc0, 0x40);
        static const QColor hot(0xd0, 0x40, 0x38);
        return e < 0.5 ? lerp(cold, mid, e * 2.0) : lerp(mid, hot, (e - 0.5) * 2.0);
    }
    }
    return theme::background();
}

void ByteMapStrip::rebuildCache() {
    if (width() <= 0 || height() <= 0 || end_ <= start_) {
        cache_ = QPixmap();
        return;
    }
    // Render each cell into a small image (cols_ wide, one image-row per screen
    // row), then let the pixmap scale to the widget width. Reading each screen
    // row's bytes once keeps this to one pass over the image.
    int rows = height();
    QImage img(cols_, rows, QImage::Format_RGB32);
    img.fill(theme::background());

    int perRow = bytesPerRow();
    int colBytes = std::max(1, perRow / cols_);
    IridaSession* s = controller_->session();
    std::vector<uint8_t> buf(static_cast<size_t>(perRow));

    for (int y = 0; y < rows; ++y) {
        uint64_t rowAddr = start_ + static_cast<uint64_t>(y) * perRow;
        if (rowAddr >= end_)
            break;
        size_t want = static_cast<size_t>(std::min<uint64_t>(perRow, end_ - rowAddr));
        size_t got = irida_read_memory(s, rowAddr, buf.data(), want);
        auto* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int c = 0; c < cols_; ++c) {
            int off = c * colBytes;
            if (static_cast<size_t>(off) >= want) {
                line[c] = theme::background().rgb();
                continue;
            }
            int len = std::min(colBytes, static_cast<int>(want) - off);
            int have = std::min(len, std::max(0, static_cast<int>(got) - off));
            QColor col =
                have > 0 ? colorForRun(buf.data() + off, have, rowAddr + off) : theme::background();
            line[c] = col.rgb();
        }
    }

    cache_ = QPixmap::fromImage(
        img.scaled(width(), rows, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

void ByteMapStrip::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), theme::background());
    if (!cache_.isNull())
        p.drawPixmap(0, 0, cache_);

    // Crosshair: the '+' set by a click (and moved while dragging), full-width
    // and full-height. Drawn as a black line with a thinner white core over it,
    // so it stays visible over any section color.
    if (pinned_.x() >= 0) {
        p.setPen(QPen(QColor(0x00, 0x00, 0x00, 0xd0), 3));
        p.drawLine(0, pinned_.y(), width(), pinned_.y());
        p.drawLine(pinned_.x(), 0, pinned_.x(), height());
        p.setPen(QPen(QColor(0xff, 0xff, 0xff, 0xf0), 1));
        p.drawLine(0, pinned_.y(), width(), pinned_.y());
        p.drawLine(pinned_.x(), 0, pinned_.x(), height());
    }
}

uint64_t ByteMapStrip::addressAt(const QPoint& pos) const {
    if (end_ <= start_)
        return 0;
    int perRow = bytesPerRow();
    int colBytes = std::max(1, perRow / cols_);
    int cellW = std::max(1, width() / cols_);
    int y = std::clamp(pos.y(), 0, height() - 1);
    int c = std::clamp(pos.x() / cellW, 0, cols_ - 1);
    uint64_t addr =
        start_ + static_cast<uint64_t>(y) * perRow + static_cast<uint64_t>(c) * colBytes;
    return addr < end_ ? addr : end_ - 1;
}

void ByteMapStrip::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton)
        return;
    // Pin the crosshair where clicked and begin a drag; navigate there too.
    dragging_ = true;
    pinned_ = event->pos();
    update();
    uint64_t addr = addressAt(event->pos());
    if (addr != 0)
        emit picked(addr);
}

void ByteMapStrip::mouseMoveEvent(QMouseEvent* event) {
    // The crosshair only exists once placed by a click; dragging moves it and
    // navigates. Plain hovering shows the tooltip but draws no crosshair.
    if (dragging_) {
        pinned_ = event->pos();
        update();
        uint64_t addr = addressAt(event->pos());
        if (addr != 0)
            emit picked(addr);
    }

    // Tooltip: the section and address under the cursor, like a legend.
    uint64_t addr = addressAt(event->pos());
    if (addr == 0)
        return;
    const IridaSection* secs = nullptr;
    size_t n = irida_sections(controller_->session(), &secs);
    QString name;
    for (size_t i = 0; i < n; ++i) {
        if (addr >= secs[i].vaddr && addr < secs[i].vaddr + secs[i].vsize) {
            name = QString::fromUtf8(secs[i].name);
            break;
        }
    }
    QString text = name.isEmpty() ? QString("0x%1").arg(addr, 0, 16)
                                  : QString("%1  0x%2").arg(name).arg(addr, 0, 16);
    QToolTip::showText(event->globalPosition().toPoint(), text, this);
}

void ByteMapStrip::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        dragging_ = false;
}
