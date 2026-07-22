// SPDX-License-Identifier: BUSL-1.1
#include "layouts/cpu_widget.hpp"
#include "layouts/disassembly_view.hpp"
#include "panels/disassembly/disassembly_panel.hpp"
#include <QVBoxLayout>

CpuWidget::CpuWidget(DebugController* controller, QWidget* parent) : QWidget(parent) {
    codeView_ = new DisassemblyView(controller, this);

    // The registers / memory / stack panels of the classic split CPU layout are
    // parked for now: memory is reachable via the code view's Hex form, the
    // stack via the Backtrace dock, and registers only mean anything with a live
    // process. The code view fills the whole area. To bring the split back,
    // restore the splitter below and the members in the header.
    //
    // registers_ = new RegistersPanel(controller, this);
    // memory_ = new MemoryPanel(controller, this);
    // stack_ = new StackPanel(controller, this);
    //
    // auto* top = new QSplitter(Qt::Horizontal, this);
    // top->addWidget(codeView_);
    // top->addWidget(registers_);
    // top->setStretchFactor(0, 3);
    // top->setStretchFactor(1, 1);
    //
    // auto* bottom = new QSplitter(Qt::Horizontal, this);
    // bottom->addWidget(memory_);
    // bottom->addWidget(stack_);
    // bottom->setStretchFactor(0, 3);
    // bottom->setStretchFactor(1, 2);
    //
    // auto* vertical = new QSplitter(Qt::Vertical, this);
    // vertical->addWidget(top);
    // vertical->addWidget(bottom);
    // vertical->setStretchFactor(0, 3);
    // vertical->setStretchFactor(1, 2);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(codeView_);
}

DisassemblyPanel* CpuWidget::disassembly() const {
    return codeView_->disassembly();
}
