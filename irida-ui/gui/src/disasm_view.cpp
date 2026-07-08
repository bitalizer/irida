// SPDX-License-Identifier: BUSL-1.1
#include "disasm_view.hpp"
#include <QHeaderView>
#include <QTableWidgetItem>

DisasmView::DisasmView(QWidget* parent) : QTableWidget(parent) {
    setColumnCount(3);
    setHorizontalHeaderLabels({"Address", "Instruction", "Annotation"});
    horizontalHeader()->setStretchLastSection(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    verticalHeader()->setVisible(false);
}

void DisasmView::load(IridaSession* session, uint64_t addr, size_t count) {
    const IridaInsnRow* rows = nullptr;
    size_t n = irida_disasm(session, addr, count, &rows);
    setRowCount(static_cast<int>(n));
    for (size_t i = 0; i < n; ++i) {
        setItem(static_cast<int>(i), 0,
                new QTableWidgetItem(QString("0x%1").arg(rows[i].address, 0, 16)));
        setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromUtf8(rows[i].text)));
        setItem(static_cast<int>(i), 2,
                new QTableWidgetItem(rows[i].annotation ? QString::fromUtf8(rows[i].annotation)
                                                        : QString()));
    }
}
