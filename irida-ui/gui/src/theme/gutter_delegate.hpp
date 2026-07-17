// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QStyledItemDelegate>
#include <functional>

// Paints the left gutter column (column 0): a filled red breakpoint dot and a
// filled current-IP arrow, drawn as real shapes rather than text glyphs. The
// delegate asks its owner, per row, what to draw via callbacks so it stays
// decoupled from the table's state storage.
class GutterDelegate : public QStyledItemDelegate {
    Q_OBJECT
  public:
    // Predicates keyed by row index.
    using RowPredicate = std::function<bool(int row)>;

    explicit GutterDelegate(QObject* parent = nullptr);
    void setIsBreakpoint(RowPredicate p) {
        isBreakpoint_ = std::move(p);
    }
    void setIsCurrentIp(RowPredicate p) {
        isCurrentIp_ = std::move(p);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

  private:
    RowPredicate isBreakpoint_;
    RowPredicate isCurrentIp_;
};
