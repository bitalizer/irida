// SPDX-License-Identifier: BUSL-1.1
#include "panels/binfmt/sections_panel.hpp"
#include "session/debug_controller.hpp"

SectionsPanel::SectionsPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Name", "Address", "Size", "Perms"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &SectionsPanel::refresh);
    connect(this, &QTableWidget::cellDoubleClicked, this, &SectionsPanel::onDoubleClicked);
}

void SectionsPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaSection* secs = nullptr;
    size_t n = irida_sections(s, &secs);
    setRows(static_cast<int>(n));
    vaddrs_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        vaddrs_.push_back(secs[i].vaddr);
        setCell(r, 0, QString::fromUtf8(secs[i].name));
        setCell(r, 1, formatAddress(secs[i].vaddr));
        setCell(r, 2, QString("0x%1").arg(secs[i].vsize, 0, 16).toUpper());
        QString perms;
        perms += (secs[i].perms & IRIDA_PERM_READ) ? 'r' : '-';
        perms += (secs[i].perms & IRIDA_PERM_WRITE) ? 'w' : '-';
        perms += (secs[i].perms & IRIDA_PERM_EXEC) ? 'x' : '-';
        setCell(r, 3, perms);
    }
}

void SectionsPanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(vaddrs_.size()))
        controller_->navigateTo(vaddrs_[static_cast<size_t>(row)]);
}
