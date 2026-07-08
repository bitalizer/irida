// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QSet>
#include <QStringList>
#include <QTableWidget>

// IridaTableView is the reusable virtual-table base for all Irida debugger
// panels (disassembly, registers, memory, stack, modules, breakpoints).
//
// It owns a left gutter column (table column 0) used to show:
//   - a "current instruction" arrow (U+2192, ->) on the active IP row
//   - a breakpoint marker dot (U+25CF, filled circle) on breakpoint rows
//
// Data columns passed to the constructor start at table column 1; callers
// address them using their own 0-based data-column index via setCell(), and
// the gutter offset is handled internally.
class IridaTableView : public QTableWidget {
    Q_OBJECT
  public:
    explicit IridaTableView(QStringList headers, QWidget* parent = nullptr);

    // Sets the row count and ensures every cell (including the gutter) has a
    // QTableWidgetItem so later updates never need to null-check.
    void setRows(int count);

    // Writes `text` into data column `col` (0-based) of `row`. Internally
    // this targets table column `col + gutterColumns()`.
    void setCell(int row, int col, const QString& text);

    // Marks `row` as the current instruction pointer row: shows the arrow in
    // the gutter and highlights the row background, scrolling it into view.
    // Pass -1 to clear the marker entirely.
    void setCurrentIpRow(int row);

    // Sets or clears the breakpoint marker + row highlight for `row`. Does
    // not clobber the current-IP row's background when row == current IP.
    void setBreakpointRow(int row, bool on);

    // Number of gutter columns reserved before the data columns begin.
    int gutterColumns() const {
        return 1;
    }

  private:
    void ensureRow(int row);
    void applyRowBackground(int row);

    int currentIpRow_ = -1;
    QSet<int> breakpointRows_;
};
