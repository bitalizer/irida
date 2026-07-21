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

    bands_.clear();
    start_ = 0;
    end_ = 0;
    for (size_t i = 0; i < n; ++i) {
        if (secs[i].vsize == 0)
            continue;
        bands_.push_back(
            {secs[i].vaddr, secs[i].vsize, secs[i].perms, QString::fromUtf8(secs[i].name)});
        uint64_t lo = secs[i].vaddr;
        uint64_t hi = secs[i].vaddr + secs[i].vsize;
        if (start_ == 0 || lo < start_)
            start_ = lo;
        if (hi > end_)
            end_ = hi;
    }
    update();
}

void OverviewBar::setCurrent(uint64_t addr) {
    current_ = addr;
    update();
}

int OverviewBar::xForAddress(uint64_t addr) const {
    if (end_ <= start_ || addr < start_ || addr > end_)
        return -1;
    double frac = static_cast<double>(addr - start_) / static_cast<double>(end_ - start_);
    return static_cast<int>(frac * width());
}

uint64_t OverviewBar::addressAt(int x) const {
    if (end_ <= start_ || width() == 0)
        return 0;
    double frac = std::clamp(static_cast<double>(x) / static_cast<double>(width()), 0.0, 1.0);
    return start_ + static_cast<uint64_t>(frac * static_cast<double>(end_ - start_));
}

void OverviewBar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), theme::background());
    if (bands_.empty() || end_ <= start_)
        return;

    const double span = static_cast<double>(end_ - start_);
    for (const Band& b : bands_) {
        int x0 = static_cast<int>((static_cast<double>(b.vaddr - start_) / span) * width());
        int x1 =
            static_cast<int>((static_cast<double>(b.vaddr + b.vsize - start_) / span) * width());
        int w = std::max(1, x1 - x0);

        QColor color;
        if (b.perms & 0x4)
            color = theme::overviewCode(); // executable
        else if (b.perms & 0x2)
            color = theme::overviewData(); // writable
        else
            color = theme::overviewReadonly();
        p.fillRect(QRect(x0, 2, w, height() - 4), color);
    }

    // Current-address marker.
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
    // Drag to scrub; hover shows the address under the cursor.
    uint64_t addr = addressAt(event->pos().x());
    if (addr != 0)
        QToolTip::showText(event->globalPosition().toPoint(), QString("0x%1").arg(addr, 0, 16),
                           this);
    if (event->buttons() & Qt::LeftButton)
        navigateToX(event->pos().x());
}
