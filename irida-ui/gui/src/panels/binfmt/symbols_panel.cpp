// SPDX-License-Identifier: BUSL-1.1
#include "panels/binfmt/symbols_panel.hpp"
#include "session/debug_controller.hpp"

SymbolsPanel::SymbolsPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Name", "Address", "Kind"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &SymbolsPanel::refresh);
    connect(this, &QTableWidget::cellDoubleClicked, this, &SymbolsPanel::onDoubleClicked);
}

void SymbolsPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaSymbol* syms = nullptr;
    size_t n = irida_symbols(s, &syms);
    setRows(static_cast<int>(n));
    addrs_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        addrs_.push_back(syms[i].addr);
        setCell(r, 0, QString::fromUtf8(syms[i].name));
        setCell(r, 1, formatAddress(syms[i].addr));
        setCell(r, 2, QString::fromUtf8(syms[i].kind));
    }
}

void SymbolsPanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(addrs_.size()))
        controller_->navigateTo(addrs_[static_cast<size_t>(row)]);
}
