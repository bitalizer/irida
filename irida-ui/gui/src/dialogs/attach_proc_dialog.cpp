// SPDX-License-Identifier: BUSL-1.1
#include "dialogs/attach_proc_dialog.hpp"
#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QTableWidget>
#include <QVBoxLayout>
#define WIN32_LEAN_AND_MEAN
// clang-format off
#include <Windows.h>
#include <TlHelp32.h>
// clang-format on

namespace {

bool matchesFilter(const QString& needleLower, uint32_t pid, const QString& nameLower) {
    if (needleLower.isEmpty())
        return true;
    if (nameLower.contains(needleLower))
        return true;
    return QString::number(pid).contains(needleLower);
}

} // namespace

AttachProcDialog::AttachProcDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Attach to Process"));
    resize(480, 420);

    filter_ = new QLineEdit(this);
    filter_->setPlaceholderText(tr("Filter by name or PID..."));

    table_ = new QTableWidget(0, 2, this);
    table_->setHorizontalHeaderLabels({tr("PID"), tr("Name")});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSortingEnabled(true);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(filter_);
    layout->addWidget(table_);
    layout->addWidget(buttons_);

    connect(filter_, &QLineEdit::textChanged, this, &AttachProcDialog::applyFilter);
    connect(table_, &QTableWidget::cellDoubleClicked, this, &AttachProcDialog::onRowActivated);
    connect(buttons_, &QDialogButtonBox::accepted, this, &AttachProcDialog::onAccept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);

    reloadProcesses();
    rebuildTable();
    filter_->setFocus();
}

void AttachProcDialog::reloadProcesses() {
    processes_.clear();

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return;

    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snap, &entry)) {
        do {
            ProcessEntry p;
            p.pid = static_cast<uint32_t>(entry.th32ProcessID);
            p.name = QString::fromWCharArray(entry.szExeFile);
            processes_.push_back(p);
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);
}

void AttachProcDialog::rebuildTable() {
    const QString needle = filter_->text().trimmed().toLower();

    table_->setSortingEnabled(false);
    table_->setRowCount(0);

    for (const ProcessEntry& p : processes_) {
        const QString nameLower = p.name.toLower();
        if (!matchesFilter(needle, p.pid, nameLower))
            continue;

        int row = table_->rowCount();
        table_->insertRow(row);

        auto* pidItem = new QTableWidgetItem();
        pidItem->setData(Qt::DisplayRole, p.pid);
        pidItem->setData(Qt::UserRole, p.pid);
        pidItem->setFlags(pidItem->flags() & ~Qt::ItemIsEditable);
        table_->setItem(row, 0, pidItem);

        auto* nameItem = new QTableWidgetItem(p.name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        table_->setItem(row, 1, nameItem);
    }

    table_->setSortingEnabled(true);
}

void AttachProcDialog::applyFilter(const QString&) {
    rebuildTable();
}

void AttachProcDialog::selectRow(int row) {
    QTableWidgetItem* item = table_->item(row, 0);
    if (!item)
        return;
    selected_pid_ = item->data(Qt::UserRole).toUInt();
}

void AttachProcDialog::onRowActivated(int row, int /*col*/) {
    selectRow(row);
    accept();
}

void AttachProcDialog::onAccept() {
    int row = table_->currentRow();
    selectRow(row);
    accept();
}
