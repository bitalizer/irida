// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QStyledItemDelegate>

// Paints a disassembly Instruction cell as syntax-highlighted tokens (mnemonic,
// register, number, braces, punctuation) using the theme palette. Row
// background / selection are painted by the base first, so this only draws the
// colored text on top. Install on the Instruction column.
class AsmItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
  public:
    explicit AsmItemDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
};
