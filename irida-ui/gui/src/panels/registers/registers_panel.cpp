// SPDX-License-Identifier: BUSL-1.1
#include "panels/registers/registers_panel.hpp"
#include "session/debug_controller.hpp"
#include "theme/palette.hpp"
#include <QBrush>

RegistersPanel::RegistersPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Register", "Value", "Hint"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &RegistersPanel::refresh);
    refresh();
}

void RegistersPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaRegister* regs = nullptr;
    size_t n = irida_registers(s, &regs);
    setRows(static_cast<int>(n));
    bool have_prev = prev_values_.size() == n;
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        setCell(r, 0, QString::fromUtf8(regs[i].name));
        setCell(r, 1, formatAddress(regs[i].value));
        setCell(r, 2, regs[i].hint ? QString::fromUtf8(regs[i].hint) : QString());

        // Register name in the amber register color; value red if it changed
        // since the last stop, else default.
        if (auto* nameIt = item(r, 0 + gutterColumns()))
            nameIt->setForeground(theme::registerName());
        bool changed = have_prev && prev_values_[i] != regs[i].value;
        if (auto* valIt = item(r, 1 + gutterColumns()))
            valIt->setForeground(changed ? QBrush(theme::changedValue()) : QBrush());
        if (auto* hintIt = item(r, 2 + gutterColumns()))
            hintIt->setForeground(theme::liveValue());
    }
    prev_values_.clear();
    for (size_t i = 0; i < n; ++i)
        prev_values_.push_back(regs[i].value);
}
