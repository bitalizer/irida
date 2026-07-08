// SPDX-License-Identifier: BUSL-1.1
#include "panels/memory/memory_panel.hpp"
#include "session/debug_controller.hpp"

namespace {
constexpr int kBytesPerRow = 16;
constexpr int kRows = 8;
constexpr size_t kWindow = static_cast<size_t>(kBytesPerRow) * kRows;
} // namespace

MemoryPanel::MemoryPanel(DebugController* controller, QWidget* parent)
    : IridaTableView({"Address", "Hex", "ASCII"}, parent), controller_(controller) {
    connect(controller_, &DebugController::stateChanged, this, &MemoryPanel::refresh);
    refresh();
}

void MemoryPanel::setBase(uint64_t addr) {
    base_ = addr;
}

void MemoryPanel::refresh() {
    IridaSession* s = controller_->session();
    uint8_t buf[kWindow];
    size_t got = irida_read_memory(s, base_, buf, kWindow);
    setRows(kRows);
    for (int r = 0; r < kRows; ++r) {
        uint64_t rowaddr = base_ + static_cast<uint64_t>(r) * kBytesPerRow;
        setCell(r, 0, QString("0x%1").arg(rowaddr, 0, 16));
        QString hex, ascii;
        for (int b = 0; b < kBytesPerRow; ++b) {
            size_t idx = static_cast<size_t>(r) * kBytesPerRow + b;
            if (idx < got) {
                uint8_t v = buf[idx];
                hex += QString("%1 ").arg(v, 2, 16, QChar('0'));
                ascii += (v >= 0x20 && v < 0x7f) ? QChar(v) : QChar('.');
            } else {
                hex += "?? ";
                ascii += ' ';
            }
        }
        setCell(r, 1, hex.trimmed());
        setCell(r, 2, ascii);
    }
}
