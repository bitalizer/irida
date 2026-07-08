// SPDX-License-Identifier: BUSL-1.1
#include "irida/mock/mock_backend.h"
#include "panels/execution/breakpoints_panel.hpp"
#include "panels/symbols/modules_panel.hpp"
#include "session/debug_controller.hpp"
#include <QApplication>
#include <QSignalSpy>
#include <QtTest/QtTest>

class TestPanelsSide : public QObject {
    Q_OBJECT
  private slots:
    void modules_list_and_navigate() {
        IridaSession* s = irida_mock_create();
        DebugController c(s);
        ModulesPanel m(&c);
        QVERIFY(m.rowCount() >= 2);
        QVERIFY(m.item(0, 1)->text().contains("ntdll")); // name col
        QSignalSpy spy(&c, &DebugController::navigationRequested);
        emit m.cellDoubleClicked(0, 1);
        QCOMPARE(spy.count(), 1);
        irida_session_destroy(s);
    }
    void breakpoints_reflect_toggle() {
        IridaSession* s = irida_mock_create();
        DebugController c(s);
        BreakpointsPanel b(&c);
        int before = b.rowCount();
        c.toggleBreakpoint(0x100b); // emits stateChanged -> panel refreshes
        QCOMPARE(b.rowCount(), before + 1);
        irida_session_destroy(s);
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestPanelsSide tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test_panels_side.moc"
