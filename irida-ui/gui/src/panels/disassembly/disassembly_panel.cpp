// SPDX-License-Identifier: BUSL-1.1
#include "panels/disassembly/disassembly_panel.hpp"
#include "session/debug_controller.hpp"

DisassemblyPanel::DisassemblyPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Address", "Bytes", "Instruction", "Live-value"}, parent),
      controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &DisassemblyPanel::refresh);
    connect(this, &QTableWidget::cellClicked, this, &DisassemblyPanel::onCellClicked);
    refresh();
}

void DisassemblyPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaInsnRow* rows = nullptr;
    size_t n = irida_disasm(s, 0, 256, &rows);
    setRows(static_cast<int>(n));
    row_addrs_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        row_addrs_.push_back(rows[i].address);
        setCell(r, 0, QString("0x%1").arg(rows[i].address, 0, 16));
        setCell(r, 1, QString()); // bytes column filled by engine later; blank for mock text
        setCell(r, 2, QString::fromUtf8(rows[i].text));
        setCell(r, 3, rows[i].annotation ? QString::fromUtf8(rows[i].annotation) : QString());
    }
    // breakpoints
    const IridaBreakpoint* bps = nullptr;
    size_t bn = irida_breakpoints(s, &bps);
    for (size_t i = 0; i < n; ++i) {
        bool on = false;
        for (size_t b = 0; b < bn; ++b)
            if (bps[b].address == row_addrs_[i])
                on = true;
        setBreakpointRow(static_cast<int>(i), on);
    }
    // current IP
    uint64_t pc = irida_pc(s);
    int ip_row = -1;
    for (size_t i = 0; i < n; ++i)
        if (row_addrs_[i] == pc)
            ip_row = static_cast<int>(i);
    setCurrentIpRow(ip_row);
}

void DisassemblyPanel::onCellClicked(int row, int col) {
    if (col != 0) // only the gutter toggles breakpoints
        return;
    if (row >= 0 && row < static_cast<int>(row_addrs_.size()))
        controller_->toggleBreakpoint(row_addrs_[static_cast<size_t>(row)]);
}
