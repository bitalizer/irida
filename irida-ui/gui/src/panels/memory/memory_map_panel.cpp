// SPDX-License-Identifier: BUSL-1.1
#include "panels/memory/memory_map_panel.hpp"
#include "session/debug_controller.hpp"

namespace {
QString permsString(uint8_t perms) {
    QString s;
    s += (perms & 0x1) ? QLatin1Char('r') : QLatin1Char('-');
    s += (perms & 0x2) ? QLatin1Char('w') : QLatin1Char('-');
    s += (perms & 0x4) ? QLatin1Char('x') : QLatin1Char('-');
    return s;
}
} // namespace

MemoryMapPanel::MemoryMapPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Start", "End", "Size", "Perms", "Name"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &MemoryMapPanel::refresh);
    connect(this, &QTableWidget::cellDoubleClicked, this, &MemoryMapPanel::onDoubleClicked);
}

void MemoryMapPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaMemMap* maps = nullptr;
    size_t n = irida_maps(s, &maps);
    setRows(static_cast<int>(n));
    starts_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        const IridaMemMap& m = maps[i];
        starts_.push_back(m.start);
        setCell(r, 0, formatAddress(m.start));
        setCell(r, 1, formatAddress(m.end));
        setCell(r, 2, QString("0x%1").arg(m.end - m.start, 0, 16).toUpper());
        setCell(r, 3, permsString(m.perms));
        setCell(r, 4, QString::fromUtf8(m.name));
    }
}

void MemoryMapPanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(starts_.size()))
        controller_->navigateTo(starts_[static_cast<size_t>(row)]);
}
