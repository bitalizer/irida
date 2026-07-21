// SPDX-License-Identifier: BUSL-1.1
#include "panels/analysis/functions_panel.hpp"
#include "session/debug_controller.hpp"

FunctionsPanel::FunctionsPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Name", "Address", "Size", "Blocks"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &FunctionsPanel::refresh);
    connect(this, &QTableWidget::cellDoubleClicked, this, &FunctionsPanel::onDoubleClicked);
}

void FunctionsPanel::refresh() {
    IridaSession* s = controller_->session();
    irida_analyze(s);
    const IridaFunction* fns = nullptr;
    size_t n = irida_functions(s, &fns);
    setRows(static_cast<int>(n));
    addrs_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        addrs_.push_back(fns[i].addr);
        setCell(r, 0, QString::fromUtf8(fns[i].name));
        setCell(r, 1, formatAddress(fns[i].addr));
        setCell(r, 2, QString("0x%1").arg(fns[i].size, 0, 16).toUpper());
        setCell(r, 3, QString::number(fns[i].block_count));
    }
}

void FunctionsPanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(addrs_.size()))
        controller_->navigateTo(addrs_[static_cast<size_t>(row)]);
}
