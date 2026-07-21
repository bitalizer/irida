// SPDX-License-Identifier: BUSL-1.1
#include "dialogs/load_options_dialog.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSlider>
#include <QVBoxLayout>

namespace {

// The per-pass toggles shown under the level selector. These describe
// analysis passes the engine does not expose yet; they are displayed disabled
// for parity and enabled individually as each pass lands.
const char* kAnalysisPasses[] = {
    "Analyze all symbols",
    "Analyze instructions for references",
    "Analyze function calls",
    "Analyze all basic blocks",
    "Autoname functions based on context",
    "Emulate code to find computed references",
    "Analyze all consecutive functions",
    "Type and Argument matching analysis",
    "Analyze code after trap-sleds",
};

QComboBox* autoCombo(QWidget* parent, const QStringList& items, bool enabled) {
    auto* combo = new QComboBox(parent);
    combo->addItems(items);
    combo->setEnabled(enabled);
    return combo;
}

} // namespace

LoadOptionsDialog::LoadOptionsDialog(const QString& programPath, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Load Options");
    setMinimumWidth(560);

    auto* layout = new QVBoxLayout(this);

    auto* programRow = new QFormLayout();
    auto* programEdit = new QLineEdit("file://" + programPath, this);
    programEdit->setReadOnly(true);
    programRow->addRow("Program:", programEdit);
    layout->addLayout(programRow);

    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    layout->addWidget(separator);

    analysisEnabled_ = new QCheckBox("Analysis: Enabled", this);
    analysisEnabled_->setChecked(true);
    layout->addWidget(analysisEnabled_);

    auto* levelLabel = new QLabel("Level: Auto-Analysis", this);
    layout->addWidget(levelLabel);

    levelSlider_ = new QSlider(Qt::Horizontal, this);
    levelSlider_->setRange(0, 3); // None / Auto / Experimental / Advanced
    levelSlider_->setValue(1);
    levelSlider_->setTickPosition(QSlider::TicksBelow);
    levelSlider_->setTickInterval(1);
    layout->addWidget(levelSlider_);

    auto* ticks = new QHBoxLayout();
    for (const char* name : {"None", "Auto", "Experimental", "Advanced"}) {
        auto* l = new QLabel(name, this);
        ticks->addWidget(l);
        ticks->addStretch();
    }
    layout->addLayout(ticks);

    connect(levelSlider_, &QSlider::valueChanged, this, [levelLabel](int v) {
        static const char* names[] = {"None", "Auto-Analysis", "Experimental", "Advanced"};
        levelLabel->setText(QString("Level: %1").arg(names[v]));
    });

    // Per-pass toggles (parity placeholders, disabled until implemented).
    auto* passes = new QListWidget(this);
    for (const char* pass : kAnalysisPasses) {
        auto* item = new QListWidgetItem(pass, passes);
        item->setFlags(Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    passes->setEnabled(false);
    layout->addWidget(passes, 1);
    connect(analysisEnabled_, &QCheckBox::toggled, this, [this, passes, levelLabel](bool on) {
        levelSlider_->setEnabled(on);
        levelLabel->setEnabled(on);
    });

    // Advanced CPU options. The engine targets x86-64 PE for now, so the
    // detected values are shown but not yet user-selectable.
    auto* cpuGroup = new QGroupBox("Advanced options", this);
    cpuGroup->setCheckable(true);
    cpuGroup->setChecked(false);
    auto* cpuForm = new QFormLayout(cpuGroup);
    archCombo_ = autoCombo(cpuGroup, {"Auto", "x86"}, false);
    bitsCombo_ = autoCombo(cpuGroup, {"Auto", "64", "32"}, false);
    endianCombo_ = autoCombo(cpuGroup, {"Auto", "little", "big"}, false);
    formatCombo_ = autoCombo(cpuGroup, {"Auto", "PE"}, false);
    cpuForm->addRow("Architecture:", archCombo_);
    cpuForm->addRow("Bits:", bitsCombo_);
    cpuForm->addRow("Endianness:", endianCombo_);
    cpuForm->addRow("Format:", formatCombo_);
    layout->addWidget(cpuGroup);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &LoadOptionsDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void LoadOptionsDialog::onAccept() {
    options_.analyze = analysisEnabled_->isChecked();
    options_.level = static_cast<AnalysisLevel>(levelSlider_->value());
    accept();
}
