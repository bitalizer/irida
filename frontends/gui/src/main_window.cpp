// SPDX-License-Identifier: BUSL-1.1
#include "main_window.hpp"
#include "disasm_view.hpp"

MainWindow::MainWindow(IridaSession* session, QWidget* parent)
    : QMainWindow(parent), disasm_(new DisasmView(this)) {
    setWindowTitle("Irida");
    setCentralWidget(disasm_);
    disasm_->load(session, 0x1000, 16);
    resize(900, 500);
}
