// SPDX-License-Identifier: BUSL-1.1
#include "widgets/irida_table_view.hpp"
#include <QTableWidgetItem>
#include <QtTest/QtTest>

class TestTableView : public QObject {
    Q_OBJECT
  private slots:
    void gutterReservesColumnZero();
    void setCellWritesGutterOffsetColumn();
    void formatAddressIsUppercaseFullWidth();
    void currentIpRowStateAndClearsPrevious();
    void breakpointRowStateTogglesAndBackground();
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

void TestTableView::formatAddressIsUppercaseFullWidth() {
    // 64-bit, uppercase, zero-padded to 16 hex digits, no 0x prefix.
    QCOMPARE(IridaTableView::formatAddress(0x00007FF751FA2440ULL), QString("00007FF751FA2440"));
    QCOMPARE(IridaTableView::formatAddress(0), QString("0000000000000000"));
    QCOMPARE(IridaTableView::formatAddress(0xABCDEFULL), QString("0000000000ABCDEF"));
}

void TestTableView::currentIpRowStateAndClearsPrevious() {
    // The IP arrow is now drawn by the gutter delegate from row STATE, not glyph
    // text; assert the state the delegate reads.
    IridaTableView view({"Address"});
    view.setRows(3);

    view.setCurrentIpRow(1);
    QVERIFY(view.isCurrentIpRow(1));

    view.setCurrentIpRow(2);
    QVERIFY(!view.isCurrentIpRow(1));
    QVERIFY(view.isCurrentIpRow(2));

    view.setCurrentIpRow(-1);
    QVERIFY(!view.isCurrentIpRow(2));
}

void TestTableView::breakpointRowStateTogglesAndBackground() {
    IridaTableView view({"Address"});
    view.setRows(2);

    view.setBreakpointRow(0, true);
    QVERIFY(view.isBreakpointRow(0));
    QVERIFY(view.item(0, 0)->background().color() != QColor(Qt::transparent));

    view.setBreakpointRow(0, false);
    QVERIFY(!view.isBreakpointRow(0));
}

void TestTableView::breakpointDoesNotClobberIpBackground() {
    IridaTableView view({"Address"});
    view.setRows(2);

    view.setCurrentIpRow(0);
    QColor ipColor = view.item(0, 0)->background().color();

    // Marking the same row as a breakpoint must not hide the current-IP
    // highlight: the row background stays the IP color (the delegate overlays
    // the breakpoint dot on the gutter regardless).
    view.setBreakpointRow(0, true);
    QVERIFY(view.isBreakpointRow(0));
    QCOMPARE(view.item(0, 0)->background().color(), ipColor);

    view.setBreakpointRow(0, false);
    QCOMPARE(view.item(0, 0)->background().color(), ipColor);
}

QTEST_MAIN(TestTableView)
#include "test_table_view.moc"
