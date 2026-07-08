// SPDX-License-Identifier: BUSL-1.1
#include "theme/icons.hpp"
#include <QApplication>
#include <QtTest/QtTest>

class TestIcons : public QObject {
    Q_OBJECT
  private slots:
    void known_icon_loads_non_null() {
        QIcon i = icons::load("play");
        QVERIFY(!i.isNull());
        // guard against "non-null but empty": the icon must yield a real pixmap
        QVERIFY(!i.pixmap(20, 20).isNull());
    }
    void missing_icon_is_safe() {
        QIcon i = icons::load("does-not-exist");
        QVERIFY(i.isNull()); // safe: null, no crash
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestIcons tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test_icons.moc"
