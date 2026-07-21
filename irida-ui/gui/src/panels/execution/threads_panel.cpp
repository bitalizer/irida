// SPDX-License-Identifier: BUSL-1.1
#include "panels/execution/threads_panel.hpp"
#include "session/debug_controller.hpp"
#include "theme/palette.hpp"
#include <QBrush>
#include <QFont>

ThreadsPanel::ThreadsPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"TID", "PC", "Current"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &ThreadsPanel::refresh);
    connect(this, &QTableWidget::cellDoubleClicked, this, &ThreadsPanel::onDoubleClicked);
}

void ThreadsPanel::refresh() {
    IridaSession* s = controller_->session();
    const IridaThread* threads = nullptr;
    size_t n = irida_threads(s, &threads);
    setRows(static_cast<int>(n));
    pcs_.clear();
    for (size_t i = 0; i < n; ++i) {
        int r = static_cast<int>(i);
        pcs_.push_back(threads[i].pc);
        setCell(r, 0, QString::number(threads[i].tid));
        setCell(r, 1, formatAddress(threads[i].pc));
        setCell(r, 2, threads[i].current ? "*" : "");

        bool current = threads[i].current != 0;
        for (int col = 0; col < 3; ++col) {
            QTableWidgetItem* it = item(r, col + gutterColumns());
            if (!it)
                continue;
            it->setBackground(current ? QBrush(theme::currentIpBackground()) : QBrush());
            QFont f = it->font();
            f.setBold(current);
            it->setFont(f);
        }
    }
}

void ThreadsPanel::onDoubleClicked(int row, int /*col*/) {
    if (row >= 0 && row < static_cast<int>(pcs_.size()))
        controller_->navigateTo(pcs_[static_cast<size_t>(row)]);
}
