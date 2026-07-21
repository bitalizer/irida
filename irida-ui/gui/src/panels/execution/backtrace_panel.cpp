// SPDX-License-Identifier: BUSL-1.1
#include "panels/execution/backtrace_panel.hpp"
#include "session/debug_controller.hpp"

BacktracePanel::BacktracePanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"#", "PC", "Frame"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &BacktracePanel::refresh);
    connect(this, &QTableWidget::cellDoubleClicked, this, &BacktracePanel::onDoubleClicked);
}

void BacktracePanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaFrame* frames = nullptr;
    size_t n = irida_backtrace(s, &frames);
    setRows(static_cast<int>(n));
    pcs_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        pcs_.push_back(frames[i].pc);
        setCell(r, 0, QString::number(i));
        setCell(r, 1, formatAddress(frames[i].pc));
        setCell(r, 2, formatAddress(frames[i].frame_ptr));
    }
}

void BacktracePanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(pcs_.size()))
        controller_->navigateTo(pcs_[static_cast<size_t>(row)]);
}
