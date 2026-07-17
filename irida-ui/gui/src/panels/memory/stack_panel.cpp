// SPDX-License-Identifier: BUSL-1.1
#include "panels/memory/stack_panel.hpp"
#include "session/debug_controller.hpp"
#include "theme/palette.hpp"
#include <QBrush>
#include <cstring>
#include <string_view>

namespace {
constexpr int kSlots = 12;
}

StackPanel::StackPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Address", "Value", "Hint"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &StackPanel::refresh);
    refresh();
}

void StackPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaRegister* regs = nullptr;
    size_t n = irida_registers(s, &regs);
    uint64_t rsp = 0;
    for (size_t i = 0; i < n; ++i)
        if (std::string_view(regs[i].name) == "rsp")
            rsp = regs[i].value;

    setRows(kSlots);
    for (int i = 0; i < kSlots; ++i) {
        uint64_t addr = rsp + static_cast<uint64_t>(i) * 8;
        uint8_t buf[8] = {0};
        size_t got = irida_read_memory(s, addr, buf, sizeof(buf));
        uint64_t val = 0;
        if (got == sizeof(buf))
            std::memcpy(&val, buf, sizeof(val));
        setCell(i, 0, formatAddress(addr));
        setCell(i, 1, formatAddress(val));
        setCell(i, 2, QString());
        // The RSP slot (row 0) gets an accent address; other slots recede.
        if (auto* a = item(i, 0 + gutterColumns()))
            a->setForeground(i == 0 ? QBrush(theme::rspMarker()) : QBrush(theme::address()));
        if (auto* v = item(i, 1 + gutterColumns()))
            v->setForeground(theme::defaultText());
    }
}
