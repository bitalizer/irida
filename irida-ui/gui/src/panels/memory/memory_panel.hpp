// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "widgets/irida_table_view.hpp"
#include <cstdint>
class DebugController;

class MemoryPanel : public IridaTableView {
    Q_OBJECT
  public:
    explicit MemoryPanel(DebugController* controller, QWidget* parent = nullptr);
    void setBase(uint64_t addr);
  public slots:
    void refresh();

  private:
    DebugController* controller_;
    uint64_t base_ = 0x1000;
};
