// SPDX-License-Identifier: BUSL-1.1
#include "irida/mock/mock_backend.h"
#include "session/debug_controller.hpp"
#include <QApplication>
#include <QSignalSpy>
#include <QtTest/QtTest>

class TestDebugController : public QObject {
    Q_OBJECT
  private slots:
    void step_emits_state_changed() {
        IridaSession* s = irida_mock_create();
        DebugController c(s);
        QSignalSpy spy(&c, &DebugController::stateChanged);
        uint64_t pc0 = irida_pc(s);
        c.stepInto();
        QCOMPARE(spy.count(), 1);
        QVERIFY(irida_pc(s) != pc0);
        irida_session_destroy(s);
    }
    void navigate_emits_request() {
        IridaSession* s = irida_mock_create();
        DebugController c(s);
        QSignalSpy spy(&c, &DebugController::navigationRequested);
        c.navigateTo(0x1004);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toULongLong(), 0x1004ull);
        irida_session_destroy(s);
    }
    void toggle_breakpoint_roundtrips() {
        IridaSession* s = irida_mock_create();
        DebugController c(s);
        const IridaBreakpoint* bps = nullptr;
        size_t before = irida_breakpoints(s, &bps);
        c.toggleBreakpoint(0x100b);
        size_t after = irida_breakpoints(s, &bps);
        QCOMPARE(after, before + 1);
        irida_session_destroy(s);
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestDebugController tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test_debug_controller.moc"
