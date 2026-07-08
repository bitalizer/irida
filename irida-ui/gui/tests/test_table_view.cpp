// SPDX-License-Identifier: BUSL-1.1
#include "widgets/irida_table_view.hpp"
#include <QTableWidgetItem>
#include <QtTest/QtTest>

namespace {
constexpr const char* kIpArrow = "→";
constexpr const char* kBreakpointDot = "●";
} // namespace

class TestTableView : public QObject {
    Q_OBJECT
  private slots:
    void gutterReservesColumnZero();
    void setCellWritesGutterOffsetColumn();
    void currentIpRowShowsArrowAndClearsPrevious();
    void breakpointRowTogglesDotAndBackground();
    void breakpointDoesNotClobberIpBackground();
};

void TestTableView::gutterReservesColumnZero() {
    IridaTableView view({"Address", "Instruction"});
    QCOMPARE(view.gutterColumns(), 1);
    QCOMPARE(view.columnCount(), 3); // gutter + 2 data columns
}

void TestTableView::setCellWritesGutterOffsetColumn() {
    IridaTableView view({"Address", "Instruction"});
    view.setRows(2);
    view.setCell(0, 0, "0x1000");
    view.setCell(0, 1, "mov eax, ebx");

    // Data column 0 lands at table column 1 (gutter offset).
    QCOMPARE(view.item(0, 1)->text(), QString("0x1000"));
    QCOMPARE(view.item(0, 2)->text(), QString("mov eax, ebx"));
    // Gutter column itself starts empty.
    QVERIFY(view.item(0, 0)->text().isEmpty());
}

void TestTableView::currentIpRowShowsArrowAndClearsPrevious() {
    IridaTableView view({"Address"});
    view.setRows(3);

    view.setCurrentIpRow(1);
    QCOMPARE(view.item(1, 0)->text(), QString::fromUtf8(kIpArrow));

    view.setCurrentIpRow(2);
    QVERIFY(view.item(1, 0)->text().isEmpty());
    QCOMPARE(view.item(2, 0)->text(), QString::fromUtf8(kIpArrow));

    view.setCurrentIpRow(-1);
    QVERIFY(view.item(2, 0)->text().isEmpty());
}

void TestTableView::breakpointRowTogglesDotAndBackground() {
    IridaTableView view({"Address"});
    view.setRows(2);

    view.setBreakpointRow(0, true);
    QCOMPARE(view.item(0, 0)->text(), QString::fromUtf8(kBreakpointDot));
    QVERIFY(view.item(0, 0)->background().color() != QColor(Qt::transparent));

    view.setBreakpointRow(0, false);
    QVERIFY(view.item(0, 0)->text().isEmpty());
}

void TestTableView::breakpointDoesNotClobberIpBackground() {
    IridaTableView view({"Address"});
    view.setRows(2);

    view.setCurrentIpRow(0);
    QColor ipColor = view.item(0, 0)->background().color();

    // Marking the same row as a breakpoint must not erase the IP row's
    // distinguishing background/arrow relationship silently: the gutter
    // still shows the breakpoint dot, and the row keeps a highlighted
    // (non-default) background.
    view.setBreakpointRow(0, true);
    QCOMPARE(view.item(0, 0)->text(), QString::fromUtf8(kBreakpointDot));
    QVERIFY(view.item(0, 0)->background().color() != QColor(Qt::transparent));
    QVERIFY(view.item(0, 0)->background().color() != ipColor);

    // Clearing the breakpoint on the still-current IP row restores the IP
    // background instead of leaving it default/uncolored.
    view.setBreakpointRow(0, false);
    QCOMPARE(view.item(0, 0)->background().color(), ipColor);
}

QTEST_MAIN(TestTableView)
#include "test_table_view.moc"
