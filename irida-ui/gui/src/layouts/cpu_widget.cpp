// SPDX-License-Identifier: BUSL-1.1
#include "layouts/cpu_widget.hpp"
#include "panels/disassembly/disassembly_panel.hpp"
#include "panels/memory/memory_panel.hpp"
#include "panels/memory/stack_panel.hpp"
#include "panels/overview/overview_bar.hpp"
#include "panels/registers/registers_panel.hpp"
#include <QSplitter>
#include <QVBoxLayout>

CpuWidget::CpuWidget(DebugController* controller, QWidget* parent) : QWidget(parent) {
    overview_ = new OverviewBar(controller, this);
    disasm_ = new DisassemblyPanel(controller, this);
    registers_ = new RegistersPanel(controller, this);
    memory_ = new MemoryPanel(controller, this);
    stack_ = new StackPanel(controller, this);

    auto* top = new QSplitter(Qt::Horizontal, this);
    top->addWidget(disasm_);
    top->addWidget(registers_);
    top->setStretchFactor(0, 3);
    top->setStretchFactor(1, 1);

    auto* bottom = new QSplitter(Qt::Horizontal, this);
    bottom->addWidget(memory_);
    bottom->addWidget(stack_);
    bottom->setStretchFactor(0, 3);
    bottom->setStretchFactor(1, 2);

    auto* vertical = new QSplitter(Qt::Vertical, this);
    vertical->addWidget(top);
    vertical->addWidget(bottom);
    vertical->setStretchFactor(0, 3);
    vertical->setStretchFactor(1, 2);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(overview_);
    layout->addWidget(vertical);
}
