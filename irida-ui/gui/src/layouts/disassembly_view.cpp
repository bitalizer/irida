// SPDX-License-Identifier: BUSL-1.1
#include "layouts/disassembly_view.hpp"
#include "panels/disassembly/disassembly_panel.hpp"
#include "panels/graph/graph_view.hpp"
#include "panels/memory/memory_panel.hpp"
#include "panels/overview/byte_overview_panel.hpp"
#include "theme/palette.hpp"
#include <QAction>
#include <QHBoxLayout>
#include <QMenu>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

DisassemblyView::DisassemblyView(DebugController* controller, QWidget* parent)
    : QWidget(parent), controller_(controller) {
    linear_ = new DisassemblyPanel(controller_, this);
    graph_ = new GraphView(controller_, this);
    hex_ = new MemoryPanel(controller_, this);
    bytes_ = new ByteOverviewPanel(controller_, this);

    stack_ = new QStackedWidget(this);
    stack_->addWidget(bytes_); // index matches View enum order
    stack_->addWidget(hex_);
    stack_->addWidget(graph_);
    stack_->addWidget(linear_);

    buildHeader();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto* header = new QWidget(this);
    header->setObjectName("DisassemblyHeader");
    header->setStyleSheet(
        QString("#DisassemblyHeader { background: %1; }").arg(theme::headerBackground().name()));
    auto* hl = new QHBoxLayout(header);
    hl->setContentsMargins(6, 2, 6, 2);
    hl->setSpacing(4);
    hl->addWidget(sourceButton_);
    hl->addWidget(viewButton_);
    hl->addWidget(reprButton_);
    hl->addStretch();
    layout->addWidget(header);
    layout->addWidget(stack_);

    selectView(View::Linear);
}

void DisassemblyView::buildHeader() {
    auto makeButton = [this](const QString& text) {
        auto* b = new QToolButton(this);
        b->setText(text);
        b->setPopupMode(QToolButton::InstantPopup);
        b->setToolButtonStyle(Qt::ToolButtonTextOnly);
        b->setAutoRaise(true);
        return b;
    };

    // Source: PE works; RAW is a placeholder until raw-byte parsing exists.
    sourceButton_ = makeButton("PE");
    auto* sourceMenu = new QMenu(sourceButton_);
    sourceMenu->addAction("PE");
    QAction* raw = sourceMenu->addAction("RAW");
    raw->setEnabled(false);
    sourceButton_->setMenu(sourceMenu);

    // View form: picks which widget is shown, and whether a representation
    // dropdown applies (Graph and Linear have representations; Bytes/Hex do not).
    viewButton_ = makeButton("Linear");
    auto* viewMenu = new QMenu(viewButton_);
    viewMenu->addAction("Bytes", this, [this] { selectView(View::Bytes); });
    viewMenu->addAction("Hex", this, [this] { selectView(View::Hex); });
    viewMenu->addAction("Graph", this, [this] { selectView(View::Graph); });
    viewMenu->addAction("Linear", this, [this] { selectView(View::Linear); });
    viewButton_->setMenu(viewMenu);

    // Representation: Disassembly is live; IL and Pseudo C are disabled until
    // their engines land (Pseudo C unlocks with the decompiler).
    reprButton_ = makeButton("Disassembly");
    auto* reprMenu = new QMenu(reprButton_);
    reprMenu->addAction("Disassembly");
    QAction* llil = reprMenu->addAction("Low Level IL");
    llil->setEnabled(false);
    QAction* pseudo = reprMenu->addAction("Pseudo C");
    pseudo->setEnabled(false);
    reprButton_->setMenu(reprMenu);
}

void DisassemblyView::selectView(View v) {
    view_ = v;
    switch (v) {
    case View::Bytes:
        stack_->setCurrentWidget(bytes_);
        viewButton_->setText("Bytes");
        bytes_->refresh();
        break;
    case View::Hex:
        stack_->setCurrentWidget(hex_);
        viewButton_->setText("Hex");
        hex_->refresh();
        break;
    case View::Graph:
        stack_->setCurrentWidget(graph_);
        viewButton_->setText("Graph");
        graph_->refresh();
        break;
    case View::Linear:
        stack_->setCurrentWidget(linear_);
        viewButton_->setText("Linear");
        linear_->refresh();
        break;
    }
    updateReprVisibility();
}

void DisassemblyView::updateReprVisibility() {
    // The representation dropdown only applies to Graph and Linear.
    bool hasRepr = (view_ == View::Graph || view_ == View::Linear);
    reprButton_->setVisible(hasRepr);
}
