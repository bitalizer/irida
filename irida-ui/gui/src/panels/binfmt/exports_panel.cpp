// SPDX-License-Identifier: BUSL-1.1
#include "panels/binfmt/exports_panel.hpp"
#include "session/debug_controller.hpp"

ExportsPanel::ExportsPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Name", "Address", "Ordinal"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &ExportsPanel::refresh);
    connect(this, &QTableWidget::cellDoubleClicked, this, &ExportsPanel::onDoubleClicked);
}

void ExportsPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaExport* exports = nullptr;
    size_t n = irida_exports(s, &exports);
    setRows(static_cast<int>(n));
    addrs_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        addrs_.push_back(exports[i].addr);
        setCell(r, 0, QString::fromUtf8(exports[i].name));
        setCell(r, 1, formatAddress(exports[i].addr));
        setCell(r, 2, QString::number(exports[i].ordinal));
    }
}

void ExportsPanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(addrs_.size()))
        controller_->navigateTo(addrs_[static_cast<size_t>(row)]);
}
