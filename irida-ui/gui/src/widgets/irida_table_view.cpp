// SPDX-License-Identifier: BUSL-1.1
#include "widgets/irida_table_view.hpp"
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHeaderView>
#include <QTableWidgetItem>

namespace {
constexpr const char* kIpArrow = "→";       // ->
constexpr const char* kBreakpointDot = "●"; // filled circle

const QColor kIpBackground(60, 90, 140);
const QColor kBreakpointBackground(140, 40, 40);
const QColor kDefaultBackground(Qt::transparent);
} // namespace

IridaTableView::IridaTableView(QStringList headers, QWidget* parent) : QTableWidget(parent) {
    setColumnCount(headers.size() + gutterColumns());

    QStringList allHeaders;
    allHeaders << QString();
    allHeaders += headers;
    setHorizontalHeaderLabels(allHeaders);

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->resizeSection(0, 24);
    verticalHeader()->setVisible(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    QFont mono("Consolas");
    mono.setStyleHint(QFont::Monospace);
    setFont(mono);
}

void IridaTableView::ensureRow(int row) {
    for (int col = 0; col < columnCount(); ++col) {
        if (!item(row, col))
            setItem(row, col, new QTableWidgetItem());
    }
}

void IridaTableView::setRows(int count) {
    setRowCount(count);
    for (int row = 0; row < count; ++row)
        ensureRow(row);
}

void IridaTableView::setCell(int row, int col, const QString& text) {
    ensureRow(row);
    int tableCol = col + gutterColumns();
    QTableWidgetItem* it = item(row, tableCol);
    if (!it) {
        it = new QTableWidgetItem();
        setItem(row, tableCol, it);
    }
    it->setText(text);
}

void IridaTableView::applyRowBackground(int row) {
    QColor bg = kDefaultBackground;
    if (row == currentIpRow_)
        bg = kIpBackground;
    if (breakpointRows_.contains(row))
        bg = kBreakpointBackground;

    for (int col = 0; col < columnCount(); ++col) {
        ensureRow(row);
        QTableWidgetItem* it = item(row, col);
        if (it)
            it->setBackground(QBrush(bg));
    }
}

void IridaTableView::setCurrentIpRow(int row) {
    int previous = currentIpRow_;
    currentIpRow_ = row;

    if (previous >= 0 && previous < rowCount()) {
        ensureRow(previous);
        QTableWidgetItem* gutter = item(previous, 0);
        if (gutter)
            gutter->setText(QString());
        applyRowBackground(previous);
    }

    if (row >= 0 && row < rowCount()) {
        ensureRow(row);
        QTableWidgetItem* gutter = item(row, 0);
        if (gutter)
            gutter->setText(QString::fromUtf8(kIpArrow));
        applyRowBackground(row);
        scrollToItem(item(row, 0));
    }
}

void IridaTableView::setBreakpointRow(int row, bool on) {
    if (row < 0 || row >= rowCount())
        return;
    ensureRow(row);

    if (on)
        breakpointRows_.insert(row);
    else
        breakpointRows_.remove(row);

    QTableWidgetItem* gutter = item(row, 0);
    if (gutter)
        gutter->setText(on ? QString::fromUtf8(kBreakpointDot) : QString());

    applyRowBackground(row);
}
