// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "widgets/irida_table_view.hpp"
#include <cstdint>
#include <vector>
class DebugController;

class FunctionsPanel : public IridaTableView {
    Q_OBJECT
  public:
    explicit FunctionsPanel(DebugController* controller, QWidget* parent = nullptr);
  public slots:
    void refresh();

  private slots:
    void onDoubleClicked(int row, int col);

  private:
    DebugController* controller_;
    std::vector<uint64_t> addrs_;
};
