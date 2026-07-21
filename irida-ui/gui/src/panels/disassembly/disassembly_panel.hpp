// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "widgets/irida_table_view.hpp"
#include <cstdint>
#include <vector>
class DebugController;

class DisassemblyPanel : public IridaTableView {
    Q_OBJECT
  public:
    explicit DisassemblyPanel(DebugController* controller, QWidget* parent = nullptr);
  public slots:
    void refresh();
    void setBase(uint64_t addr);

  private slots:
    void onCellClicked(int row, int col);

  private:
    DebugController* controller_;
    std::vector<uint64_t> row_addrs_;
    uint64_t base_ = 0;
};
