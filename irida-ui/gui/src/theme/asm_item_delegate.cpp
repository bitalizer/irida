// SPDX-License-Identifier: BUSL-1.1
#include "theme/asm_item_delegate.hpp"
#include "theme/asm_tokenizer.hpp"
#include "theme/palette.hpp"
#include <QApplication>
#include <QPainter>
#include <QStyle>

namespace {

QColor colorFor(theme::TokenKind kind) {
    switch (kind) {
    case theme::TokenKind::Mnemonic:
        return theme::mnemonic();
    case theme::TokenKind::Register:
        return theme::registerName();
    case theme::TokenKind::Number:
        return theme::number();
    case theme::TokenKind::Brace:
        return theme::memoryBrace();
    case theme::TokenKind::Punctuation:
        return theme::punctuation();
    case theme::TokenKind::Whitespace:
    case theme::TokenKind::Text:
    default:
        return theme::defaultText();
    }
}

} // namespace

AsmItemDelegate::AsmItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void AsmItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                            const QModelIndex& index) const {
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    const QString text = opt.text;

    // Draw ONLY the background/selection primitive via the style; never let the
    // base paint the item text (it would show through under our colored tokens
    // as a duplicate). We render the text ourselves below.
    opt.text.clear();
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, w);

    if (text.isEmpty())
        return;

    painter->save();
    painter->setFont(opt.font);
    const QFontMetrics fm(opt.font);

    // left-align inside the cell with a small padding, vertically centered
    const int pad = 4;
    int x = opt.rect.left() + pad;
    const int baseY = opt.rect.top() + (opt.rect.height() + fm.ascent() - fm.descent()) / 2;

    const QVector<theme::Token> tokens = theme::tokenize(text);
    for (const theme::Token& tok : tokens) {
        painter->setPen(colorFor(tok.kind));
        painter->drawText(x, baseY, tok.text);
        x += fm.horizontalAdvance(tok.text);
        if (x > opt.rect.right())
            break;
    }
    painter->restore();
}
