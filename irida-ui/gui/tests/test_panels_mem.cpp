// SPDX-License-Identifier: BUSL-1.1
#include "irida/mock/mock_backend.h"
#include "panels/memory/memory_panel.hpp"
#include "panels/memory/stack_panel.hpp"
#include "session/debug_controller.hpp"
#include <QApplication>
#include <QtTest/QtTest>

class TestPanelsMem : public QObject {
    Q_OBJECT
  private slots:
    void memory_renders_hex_rows() {
        IridaSession* s = irida_mock_create();
        DebugController c(s);
        MemoryPanel m(&c);
        m.setBase(0x1000);
        m.refresh();
        QVERIFY(m.rowCount() >= 1);
        // hex column (data col 1 -> table col 2) has 16 two-hex-digit groups
        QString hex = m.item(0, 2)->text();
        QVERIFY(hex.split(' ', Qt::SkipEmptyParts).size() == 16);
        irida_session_destroy(s);
    }
    void stack_reads_from_rsp() {
        IridaSession* s = irida_mock_create();
        DebugController c(s);
        StackPanel st(&c);
        st.refresh();
        QVERIFY(st.rowCount() >= 1);
        // first address equals rsp (0x14fe00 from the mock)
        QVERIFY(st.item(0, 1)->text().contains("14fe00"));
        irida_session_destroy(s);
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestPanelsMem tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test_panels_mem.moc"
