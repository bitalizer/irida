// SPDX-License-Identifier: BUSL-1.1
#include "theme/gutter_delegate.hpp"
#include "theme/palette.hpp"
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>

GutterDelegate::GutterDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void GutterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                           const QModelIndex& index) const {
    // Draw only the background/selection primitive; the gutter never shows text
    // (markers are shapes drawn below).
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    opt.text.clear();
    const QWidget* widget = opt.widget;
    QStyle* style = widget ? widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);

    const int row = index.row();
    const QRect r = opt.rect;
    const int cy = r.center().y();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Breakpoint: a solid red dot (IDE convention), left side of the gutter.
    if (isBreakpoint_ && isBreakpoint_(row)) {
        const int d = qMin(r.height() - 6, 12);
        const int cx = r.left() + 4 + d / 2;
        painter->setPen(Qt::NoPen);
        painter->setBrush(theme::breakpointDot());
        painter->drawEllipse(QPoint(cx, cy), d / 2, d / 2);
    }

    // Current IP: a filled right-pointing arrow (green), right side of the gutter.
    if (isCurrentIp_ && isCurrentIp_(row)) {
        const int h = qMin(r.height() - 8, 11);
        const int w = h;
        const int right = r.right() - 4;
        const int left = right - w;
        QPainterPath path;
        path.moveTo(left, cy - h / 2.0);
        path.lineTo(right, cy);
        path.lineTo(left, cy + h / 2.0);
        path.closeSubpath();
        painter->setPen(Qt::NoPen);
        painter->setBrush(theme::currentIpMarker());
        painter->drawPath(path);
    }

    painter->restore();
}
