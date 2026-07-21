// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "widgets/irida_table_view.hpp"
#include <cstdint>
#include <vector>
class DebugController;

class XrefsPanel : public IridaTableView {
    Q_OBJECT
  public:
    explicit XrefsPanel(DebugController* controller, QWidget* parent = nullptr);
  public slots:
    void refresh();
    void setTarget(uint64_t addr);

  private slots:
    void onDoubleClicked(int row, int col);

  private:
    DebugController* controller_;
    uint64_t target_ = 0;
    std::vector<uint64_t> froms_;
};
