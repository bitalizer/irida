// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <cstdint>

// IridaTableView is the reusable virtual-table base for all Irida debugger
// panels (disassembly, registers, memory, stack, modules, breakpoints).
//
// It owns a left gutter column (table column 0). The gutter's markers — a red
// breakpoint dot and a filled current-IP arrow — are drawn as real shapes by a
// GutterDelegate (installed via installGutterDelegate); the view just tracks
// which rows are marked and exposes that state to the delegate.
//
// Data columns passed to the constructor start at table column 1; callers
// address them using their own 0-based data-column index via setCell(), and
// the gutter offset is handled internally.
class IridaTableView : public QTableWidget {
    Q_OBJECT
  public:
    explicit IridaTableView(QStringList headers, QWidget* parent = nullptr);

    // Format a guest address the way a debugger shows it: uppercase, zero-padded
    // to 16 hex digits (full 64-bit), no "0x" prefix. e.g. 00007FF751FA2440.
    static QString formatAddress(uint64_t addr);

    // Install the shape-drawing gutter delegate on column 0, wired to this
    // view's IP/breakpoint state. Call once after construction.
    void installGutterDelegate();

    // Install the syntax-highlighting instruction delegate on a data column.
    void installAsmDelegate(int dataCol);

    // Row-state queries (used by the gutter delegate).
    bool isCurrentIpRow(int row) const {
        return row == currentIpRow_;
    }
    bool isBreakpointRow(int row) const {
        return breakpointRows_.contains(row);
    }

    // Sets the row count and ensures every cell (including the gutter) has a
    // QTableWidgetItem so later updates never need to null-check.
    void setRows(int count);

    // Writes `text` into data column `col` (0-based) of `row`. Internally
    // this targets table column `col + gutterColumns()`.
    void setCell(int row, int col, const QString& text);

    // Marks `row` as the current instruction pointer row: highlights the row
    // background and scrolls it into view (the gutter arrow is drawn by the
    // delegate). Pass -1 to clear the marker entirely.
    void setCurrentIpRow(int row);

    // Sets or clears the breakpoint marker + row highlight for `row`. Does
    // not clobber the current-IP row's background when row == current IP.
    void setBreakpointRow(int row, bool on);

    // Number of gutter columns reserved before the data columns begin.
    int gutterColumns() const {
        return 1;
    }

    // Shows or hides data column `col` (0-based), accounting for the gutter.
    void setDataColumnHidden(int col, bool hidden) {
        setColumnHidden(col + gutterColumns(), hidden);
    }

  private:
    void ensureRow(int row);
    void applyRowBackground(int row);

    int currentIpRow_ = -1;
    QSet<int> breakpointRows_;
};
