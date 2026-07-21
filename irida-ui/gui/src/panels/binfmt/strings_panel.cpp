// SPDX-License-Identifier: BUSL-1.1
#include "panels/binfmt/strings_panel.hpp"
#include "session/debug_controller.hpp"

StringsPanel::StringsPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Address", "String"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &StringsPanel::refresh);
    connect(this, &QTableWidget::cellDoubleClicked, this, &StringsPanel::onDoubleClicked);
}

void StringsPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaString* strs = nullptr;
    size_t n = irida_strings(s, &strs);
    setRows(static_cast<int>(n));
    addrs_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        addrs_.push_back(strs[i].addr);
        setCell(r, 0, formatAddress(strs[i].addr));
        setCell(r, 1, strs[i].text ? QString::fromUtf8(strs[i].text) : QString());
    }
}

void StringsPanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(addrs_.size()))
        controller_->navigateTo(addrs_[static_cast<size_t>(row)]);
}
