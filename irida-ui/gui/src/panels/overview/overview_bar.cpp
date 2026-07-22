// SPDX-License-Identifier: BUSL-1.1
#include "panels/overview/overview_bar.hpp"
#include "session/debug_controller.hpp"
#include "theme/palette.hpp"
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <algorithm>

namespace {
constexpr int kBarHeight = 22;
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

    // Sections are packed back-to-back by size (ignoring the gaps between their
    // virtual addresses), so the whole bar is filled and every pixel maps to a
    // real section. total_ is the summed size that a pixel position divides.
    bands_.clear();
    total_ = 0;
    for (size_t i = 0; i < n; ++i) {
        if (secs[i].vsize == 0)
            continue;
        bands_.push_back({secs[i].vaddr, secs[i].vsize, total_, QString::fromUtf8(secs[i].name)});
        total_ += secs[i].vsize;
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
    if (total_ == 0)
        return -1;
    const Band* b = bandForAddress(addr);
    if (!b)
        return -1;
    uint64_t packed = b->packedStart + (addr - b->vaddr);
    return static_cast<int>((static_cast<double>(packed) / static_cast<double>(total_)) * width());
}

uint64_t OverviewBar::addressAt(int x) const {
    if (total_ == 0 || width() == 0)
        return 0;
    double frac = std::clamp(static_cast<double>(x) / static_cast<double>(width()), 0.0, 1.0);
    uint64_t packed = static_cast<uint64_t>(frac * static_cast<double>(total_));
    // Find the band whose packed range contains this position, then convert
    // back to the section's virtual address.
    for (const Band& b : bands_) {
        if (packed >= b.packedStart && packed < b.packedStart + b.vsize)
            return b.vaddr + (packed - b.packedStart);
    }
    return bands_.empty() ? 0 : bands_.back().vaddr;
}

void OverviewBar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), theme::background());
    if (bands_.empty() || total_ == 0)
        return;

    for (size_t i = 0; i < bands_.size(); ++i) {
        const Band& b = bands_[i];
        int x0 = static_cast<int>((static_cast<double>(b.packedStart) / total_) * width());
        int x1 =
            static_cast<int>((static_cast<double>(b.packedStart + b.vsize) / total_) * width());
        int w = std::max(1, x1 - x0);
        p.fillRect(QRect(x0, 2, w, height() - 4), theme::overviewSection(static_cast<int>(i)));
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
