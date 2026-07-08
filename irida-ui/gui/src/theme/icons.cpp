// SPDX-License-Identifier: BUSL-1.1
#include "theme/icons.hpp"
#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSvgRenderer>

namespace icons {

QIcon load(const QString& name) {
    const QString path = QString(":/icons/%1.svg").arg(name);
    if (!QFile::exists(path))
        return QIcon();
    QSvgRenderer renderer(path);
    if (!renderer.isValid())
        return QIcon();
    const int sz = 20;
    QPixmap pm(sz, sz);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    renderer.render(&p);
    p.end();
    // tint the (monochrome) glyph to the palette text color
    QColor tint = QApplication::palette().color(QPalette::WindowText);
    QPixmap tinted(pm.size());
    tinted.fill(Qt::transparent);
    QPainter tp(&tinted);
    tp.drawPixmap(0, 0, pm);
    tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tp.fillRect(tinted.rect(), tint);
    tp.end();
    return QIcon(tinted);
}

} // namespace icons
