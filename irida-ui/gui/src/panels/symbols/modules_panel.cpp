// SPDX-License-Identifier: BUSL-1.1
#include "panels/symbols/modules_panel.hpp"
#include "session/debug_controller.hpp"

ModulesPanel::ModulesPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Name", "Base", "Size"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &ModulesPanel::refresh);
    connect(this, &QTableWidget::cellDoubleClicked, this, &ModulesPanel::onDoubleClicked);
}

void ModulesPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaModule* mods = nullptr;
    size_t n = irida_modules(s, &mods);
    setRows(static_cast<int>(n));
    bases_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        bases_.push_back(mods[i].base);
        setCell(r, 0, QString::fromUtf8(mods[i].name));
        setCell(r, 1, formatAddress(mods[i].base));
        setCell(r, 2, QString("0x%1").arg(mods[i].size, 0, 16).toUpper());
    }
}

void ModulesPanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(bases_.size()))
        controller_->navigateTo(bases_[static_cast<size_t>(row)]);
}
