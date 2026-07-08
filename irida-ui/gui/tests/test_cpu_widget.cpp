// SPDX-License-Identifier: BUSL-1.1
#include "irida/mock/mock_backend.h"
#include "layouts/cpu_widget.hpp"
#include "session/debug_controller.hpp"
#include <QApplication>
#include <QSplitter>
#include <QtTest/QtTest>

class TestCpuWidget : public QObject {
    Q_OBJECT
  private slots:
    void builds_with_four_panels() {
        IridaSession* s = irida_mock_create();
        DebugController c(s);
        CpuWidget w(&c);
        // it contains at least two QSplitters (nested layout)
        QVERIFY(w.findChildren<QSplitter*>().size() >= 2);
        QVERIFY(w.disassembly() != nullptr);
        irida_session_destroy(s);
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestCpuWidget tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test_cpu_widget.moc"
