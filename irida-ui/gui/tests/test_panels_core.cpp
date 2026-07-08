// SPDX-License-Identifier: BUSL-1.1
#include "irida/mock/mock_backend.h"
#include "panels/disassembly/disassembly_panel.hpp"
#include "panels/registers/registers_panel.hpp"
#include "session/debug_controller.hpp"
#include <QApplication>
#include <QtTest/QtTest>

class TestPanelsCore : public QObject {
    Q_OBJECT
  private slots:
    void disasm_shows_rows_and_marks_ip() {
        IridaSession* s = irida_mock_create();
        DebugController c(s);
        DisassemblyPanel d(&c);
        QVERIFY(d.rowCount() >= 4);
        // the row whose address == pc (0x1000) is row 0 -> gutter has arrow
        QVERIFY(d.item(0, 0)->text().contains(QChar(0x2192)));
        // instruction text is present in the Instruction column (data col 2 -> table col 3)
        QVERIFY(d.item(0, 3)->text().contains("mov"));
        irida_session_destroy(s);
    }
    void registers_flag_changes() {
        IridaSession* s = irida_mock_create();
        DebugController c(s);
        RegistersPanel r(&c);
        // snapshot rax cell text
        QString before = r.item(0, 2)->text(); // rax value col (data col 1 -> table col 2)
        c.stepInto();                          // mock mutates rax
        QString after = r.item(0, 2)->text();
        QVERIFY(before != after);
        irida_session_destroy(s);
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestPanelsCore tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test_panels_core.moc"
