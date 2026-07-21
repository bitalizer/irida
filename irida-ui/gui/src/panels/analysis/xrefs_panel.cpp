// SPDX-License-Identifier: BUSL-1.1
#include "panels/analysis/xrefs_panel.hpp"
#include "session/debug_controller.hpp"

XrefsPanel::XrefsPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"From", "To", "Type"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &XrefsPanel::refresh);
    connect(controller_, &DebugController::navigationRequested, this, &XrefsPanel::setTarget);
    connect(this, &QTableWidget::cellDoubleClicked, this, &XrefsPanel::onDoubleClicked);
}

void XrefsPanel::setTarget(uint64_t addr) {
    target_ = addr;
    refresh();
}

void XrefsPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaXref* xrefs = nullptr;
    size_t n = irida_xrefs_to(s, target_, &xrefs);
    setRows(static_cast<int>(n));
    froms_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        froms_.push_back(xrefs[i].from);
        setCell(r, 0, formatAddress(xrefs[i].from));
        setCell(r, 1, formatAddress(xrefs[i].to));
        QString type;
        switch (xrefs[i].kind) {
        case IRIDA_XREF_CALL:
            type = "Call";
            break;
        case IRIDA_XREF_JUMP:
            type = "Jump";
            break;
        case IRIDA_XREF_DATA:
            type = "Data";
            break;
        default:
            type = "?";
            break;
        }
        setCell(r, 2, type);
    }
}

void XrefsPanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(froms_.size()))
        controller_->navigateTo(froms_[static_cast<size_t>(row)]);
}
