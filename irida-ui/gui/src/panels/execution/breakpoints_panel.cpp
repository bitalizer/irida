// SPDX-License-Identifier: BUSL-1.1
#include "panels/execution/breakpoints_panel.hpp"
#include "session/debug_controller.hpp"

BreakpointsPanel::BreakpointsPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Enabled", "Address", "Type"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &BreakpointsPanel::refresh);
    connect(this, &QTableWidget::cellDoubleClicked, this, &BreakpointsPanel::onDoubleClicked);
    refresh();
}

void BreakpointsPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaBreakpoint* bps = nullptr;
    size_t n = irida_breakpoints(s, &bps);
    setRows(static_cast<int>(n));
    addrs_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        addrs_.push_back(bps[i].address);
        setCell(r, 0, bps[i].enabled ? "yes" : "no");
        setCell(r, 1, formatAddress(bps[i].address));
        setCell(r, 2, bps[i].type == IRIDA_BP_HARDWARE ? "hw" : "sw");
    }
}

void BreakpointsPanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(addrs_.size()))
        controller_->navigateTo(addrs_[static_cast<size_t>(row)]);
}
