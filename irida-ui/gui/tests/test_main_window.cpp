// SPDX-License-Identifier: BUSL-1.1
#include "app/main_window.hpp"
#include "irida/mock/mock_backend.h"
#include <QApplication>
#include <QDockWidget>
#include <QToolBar>
#include <QtTest/QtTest>

class TestMainWindow : public QObject {
    Q_OBJECT
  private slots:
    void has_toolbar_and_docks() {
        IridaSession* s = irida_mock_create();
        MainWindow w(s);
        QVERIFY(!w.findChildren<QToolBar*>().isEmpty());
        // two dock widgets: modules + breakpoints
        QVERIFY(w.findChildren<QDockWidget*>().size() >= 2);
        QVERIFY(w.centralWidget() != nullptr);
        irida_session_destroy(s);
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestMainWindow tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test_main_window.moc"
