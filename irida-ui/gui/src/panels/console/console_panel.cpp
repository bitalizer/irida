// SPDX-License-Identifier: BUSL-1.1
#include "panels/console/console_panel.hpp"
#include "session/debug_controller.hpp"

#include <QFont>
#include <QPlainTextEdit>
#include <QVBoxLayout>

ConsolePanel::ConsolePanel(DebugController* controller, QWidget* parent)
    : QWidget(parent), controller_(controller), output_(new QPlainTextEdit(this)) {
    output_->setReadOnly(true);
    output_->setMaximumBlockCount(5000);

    QFont mono("Consolas");
    mono.setStyleHint(QFont::Monospace);
    output_->setFont(mono);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(output_);

    connect(controller_, &DebugController::stateChanged, this, &ConsolePanel::onStateChanged);
}

void ConsolePanel::appendLine(const QString& text) {
    output_->appendPlainText(text);
}

void ConsolePanel::onStateChanged() {
    IridaSession* s = controller_->session();
    uint64_t pc = irida_pc(s);
    uint64_t epoch = irida_state_epoch(s);
    QString pcHex = QString("0x%1").arg(pc, 0, 16);
    appendLine(QString("stopped @ %1 (epoch %2)").arg(pcHex).arg(epoch));
}
