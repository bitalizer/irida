// SPDX-License-Identifier: BUSL-1.1
#include "widgets/irida_table_view.hpp"
#include "theme/asm_item_delegate.hpp"
#include "theme/gutter_delegate.hpp"
#include "theme/palette.hpp"
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHeaderView>
#include <QTableWidgetItem>

IridaTableView::IridaTableView(QStringList headers, QWidget* parent) : QTableWidget(parent) {
    setColumnCount(headers.size() + gutterColumns());

    QStringList allHeaders;
    allHeaders << QString();
    allHeaders += headers;
    setHorizontalHeaderLabels(allHeaders);

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->resizeSection(0, 26);
    horizontalHeader()->setHighlightSections(false);
    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(18);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setShowGrid(false);

    QFont mono("Consolas");
    mono.setStyleHint(QFont::Monospace);
    mono.setPixelSize(13);
    setFont(mono);

    // Theme the surface via a stylesheet so headers, gridlines and selection
    // all read as one system. Delegates paint token/marker colors on top.
    setStyleSheet(QString("QTableWidget {"
                          "  background-color: %1;"
                          "  color: %2;"
                          "  gridline-color: %3;"
                          "  outline: none;"
                          "  border: none;"
                          "}"
                          "QTableWidget::item { padding-left: 4px; }"
                          "QTableWidget::item:selected { background-color: %4; }"
                          "QHeaderView::section {"
                          "  background-color: %5;"
                          "  color: %6;"
                          "  border: none;"
                          "  border-right: 1px solid %3;"
                          "  padding: 2px 6px;"
                          "}")
                      .arg(theme::background().name(), theme::defaultText().name(),
                           theme::gridLine().name(), theme::selectionBackground().name(),
                           theme::headerBackground().name(), theme::headerText().name()));
}

QString IridaTableView::formatAddress(uint64_t addr) {
    return QString("%1").arg(addr, 16, 16, QChar('0')).toUpper();
}

void IridaTableView::installGutterDelegate() {
    auto* d = new GutterDelegate(this);
    d->setIsBreakpoint([this](int row) { return isBreakpointRow(row); });
    d->setIsCurrentIp([this](int row) { return isCurrentIpRow(row); });
    setItemDelegateForColumn(0, d);
}

void IridaTableView::installAsmDelegate(int dataCol) {
    setItemDelegateForColumn(dataCol + gutterColumns(), new AsmItemDelegate(this));
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
    QColor bg = Qt::transparent;
    if (breakpointRows_.contains(row))
        bg = theme::breakpointBackground();
    if (row == currentIpRow_)
        bg = theme::currentIpBackground();

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

    if (previous >= 0 && previous < rowCount())
        applyRowBackground(previous);

    if (row >= 0 && row < rowCount()) {
        applyRowBackground(row);
        scrollToItem(item(row, 0));
    }
    if (viewport())
        viewport()->update(); // repaint gutter markers
}

void IridaTableView::setBreakpointRow(int row, bool on) {
    if (row < 0 || row >= rowCount())
        return;
    ensureRow(row);

    if (on)
        breakpointRows_.insert(row);
    else
        breakpointRows_.remove(row);

    applyRowBackground(row);
    if (viewport())
        viewport()->update(); // repaint gutter markers
}
