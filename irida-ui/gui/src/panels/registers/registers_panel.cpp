// SPDX-License-Identifier: BUSL-1.1
#include "panels/registers/registers_panel.hpp"
#include "session/debug_controller.hpp"
#include <QBrush>
#include <QColor>

namespace {
const QColor kChanged(220, 150, 60); // accent for changed values
}

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
        setCell(r, 1, QString("0x%1").arg(regs[i].value, 0, 16));
        setCell(r, 2, regs[i].hint ? QString::fromUtf8(regs[i].hint) : QString());
        bool changed = have_prev && prev_values_[i] != regs[i].value;
        // value cell is data col 1 -> table col 2
        item(r, 2)->setForeground(changed ? QBrush(kChanged) : QBrush());
    }
    prev_values_.clear();
    for (size_t i = 0; i < n; ++i)
        prev_values_.push_back(regs[i].value);
}
