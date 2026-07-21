// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QDialog>
#include <cstdint>
#include <vector>
class QLineEdit;
class QTableWidget;
class QDialogButtonBox;

class AttachProcDialog : public QDialog {
    Q_OBJECT
  public:
    explicit AttachProcDialog(QWidget* parent = nullptr);

    uint32_t selectedPid() const {
        return selected_pid_;
    }

  private slots:
    void applyFilter(const QString& text);
    void onRowActivated(int row, int col);
    void onAccept();

  private:
    struct ProcessEntry {
        uint32_t pid;
        QString name;
    };

    void reloadProcesses();
    void rebuildTable();
    void selectRow(int row);

    QLineEdit* filter_;
    QTableWidget* table_;
    QDialogButtonBox* buttons_;

    std::vector<ProcessEntry> processes_;
    uint32_t selected_pid_ = 0;
};
