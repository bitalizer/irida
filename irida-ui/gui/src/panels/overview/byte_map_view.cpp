// SPDX-License-Identifier: BUSL-1.1
#include "panels/overview/byte_map_view.hpp"
#include "session/debug_controller.hpp"
#include <QVBoxLayout>
#include <algorithm>

namespace {
constexpr int kMapWidth = 64; // a slim strip beside the code view
} // namespace

ByteMapView::ByteMapView(DebugController* controller, QWidget* parent)
    : QWidget(parent), controller_(controller) {
    strip_ = new ByteMapStrip(controller_, ByteMapStrip::Mode::SectionTexture, this);
    // A click navigates the session; the crosshair is handled inside the strip
    // and follows the mouse only while hovering.
    connect(strip_, &ByteMapStrip::picked, controller_, &DebugController::navigateTo);
    connect(controller_, &DebugController::stateChanged, this, &ByteMapView::refresh);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(strip_);
    setFixedWidth(kMapWidth);
}

void ByteMapView::computeRange() {
    IridaSession* s = controller_->session();
    const IridaSection* secs = nullptr;
    size_t n = irida_sections(s, &secs);
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
}

void ByteMapView::refresh() {
    computeRange();
    strip_->setRange(start_, end_);
}

void ByteMapView::resizeEvent(QResizeEvent*) {}
