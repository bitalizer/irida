// SPDX-License-Identifier: BUSL-1.1
#include "panels/overview/overview_bar.hpp"
#include "session/debug_controller.hpp"
#include "theme/palette.hpp"
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <algorithm>
#include <cmath>

namespace {
constexpr int kBarHeight = 22;

// Compress a section's byte size into a visual weight. The square root pulls
// huge and tiny sections closer together so every section keeps a clickable
// slice of the bar, while still keeping larger sections visibly larger.
double displayWeight(uint64_t vsize) {
    return std::sqrt(static_cast<double>(vsize));
}
} // namespace

OverviewBar::OverviewBar(DebugController* controller, QWidget* parent)
    : QWidget(parent), controller_(controller) {
    setFixedHeight(kBarHeight);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    connect(controller_, &DebugController::stateChanged, this, &OverviewBar::refresh);
    connect(controller_, &DebugController::navigationRequested, this, &OverviewBar::setCurrent);
}

QSize OverviewBar::sizeHint() const {
    return QSize(400, kBarHeight);
}

void OverviewBar::refresh() {
    IridaSession* s = controller_->session();
    const IridaSection* secs = nullptr;
    size_t n = irida_sections(s, &secs);

    // Lay the sections out left to right by compressed weight (not raw byte
    // size), so the whole bar is filled and even the smallest section keeps a
    // visible band. Each band also carries a stable color index.
    bands_.clear();
    displayTotal_ = 0;
    int color = 0;
    for (size_t i = 0; i < n; ++i) {
        if (secs[i].vsize == 0)
            continue;
        double span = displayWeight(secs[i].vsize);
        bands_.push_back({secs[i].vaddr, secs[i].vsize, displayTotal_, span, color++,
                          QString::fromUtf8(secs[i].name)});
        displayTotal_ += span;
    }
    update();
}

void OverviewBar::setCurrent(uint64_t addr) {
    current_ = addr;
    update();
}

const OverviewBar::Band* OverviewBar::bandForAddress(uint64_t addr) const {
    for (const Band& b : bands_)
        if (addr >= b.vaddr && addr < b.vaddr + b.vsize)
            return &b;
    return nullptr;
}

int OverviewBar::xForAddress(uint64_t addr) const {
    if (displayTotal_ == 0)
        return -1;
    const Band* b = bandForAddress(addr);
    if (!b)
        return -1;
    // Position within the band scales by how far the address sits into the real
    // section, but the band's own width comes from its compressed span.
    double intoBand =
        b->vsize ? static_cast<double>(addr - b->vaddr) / static_cast<double>(b->vsize) : 0.0;
    double pos = b->displayStart + intoBand * b->displaySpan;
    return static_cast<int>((pos / displayTotal_) * width());
}

uint64_t OverviewBar::addressAt(int x) const {
    if (displayTotal_ == 0 || width() == 0)
        return 0;
    double frac = std::clamp(static_cast<double>(x) / static_cast<double>(width()), 0.0, 1.0);
    double pos = frac * displayTotal_;
    // Find the band covering this visual position, then map back through the
    // band's real byte range so the returned address is exact.
    for (const Band& b : bands_) {
        if (pos >= b.displayStart && pos < b.displayStart + b.displaySpan) {
            double intoBand = b.displaySpan ? (pos - b.displayStart) / b.displaySpan : 0.0;
            return b.vaddr + static_cast<uint64_t>(intoBand * static_cast<double>(b.vsize));
        }
    }
    return bands_.empty() ? 0 : bands_.back().vaddr;
}

void OverviewBar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), theme::background());
    if (bands_.empty() || displayTotal_ == 0)
        return;

    for (const Band& b : bands_) {
        int x0 = static_cast<int>((b.displayStart / displayTotal_) * width());
        int x1 = static_cast<int>(((b.displayStart + b.displaySpan) / displayTotal_) * width());
        int w = std::max(1, x1 - x0);
        p.fillRect(QRect(x0, 2, w, height() - 4), theme::overviewSection(b.colorIndex));
    }

    int mx = xForAddress(current_);
    if (mx >= 0) {
        p.setPen(QPen(theme::overviewMarker(), 2));
        p.drawLine(mx, 0, mx, height());
    }
}

void OverviewBar::navigateToX(int x) {
    uint64_t addr = addressAt(x);
    if (addr != 0)
        controller_->navigateTo(addr);
}

void OverviewBar::mousePressEvent(QMouseEvent* event) {
    navigateToX(event->pos().x());
}

void OverviewBar::mouseMoveEvent(QMouseEvent* event) {
    // Hover shows the section and address under the cursor; a held button drags
    // the position to scrub through the program.
    uint64_t addr = addressAt(event->pos().x());
    if (addr != 0) {
        const Band* b = bandForAddress(addr);
        QString name = b ? b->name : QString();
        QString text = name.isEmpty() ? QString("0x%1").arg(addr, 0, 16)
                                      : QString("%1  0x%2").arg(name).arg(addr, 0, 16);
        QToolTip::showText(event->globalPosition().toPoint(), text, this);
    }
    if (event->buttons() & Qt::LeftButton)
        navigateToX(event->pos().x());
}
