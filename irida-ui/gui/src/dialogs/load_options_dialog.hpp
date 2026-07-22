// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QDialog>
#include <QString>
class QCheckBox;
class QButtonGroup;
class QComboBox;
class QGroupBox;

// The options shown after choosing a binary, before it opens. An analysis
// toggle + depth selector drive real behavior; the CPU and per-pass options are
// presented and enabled as the engine gains the corresponding capability.
class LoadOptionsDialog : public QDialog {
    Q_OBJECT
  public:
    // Analysis depth, from cheapest to most thorough. Maps to how many roots
    // the analyzer is seeded from.
    enum class AnalysisLevel { None, Auto, Experimental, Advanced };

    struct Options {
        bool analyze = true;
        AnalysisLevel level = AnalysisLevel::Auto;
    };

    explicit LoadOptionsDialog(const QString& programPath, QWidget* parent = nullptr);

    Options options() const {
        return options_;
    }

  private slots:
    void onAccept();

  private:
    QCheckBox* analysisEnabled_;
    QButtonGroup* levelGroup_;
    QComboBox* archCombo_;
    QComboBox* bitsCombo_;
    QComboBox* endianCombo_;
    QComboBox* formatCombo_;
    Options options_;
};
