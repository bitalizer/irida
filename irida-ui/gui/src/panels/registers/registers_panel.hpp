// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "widgets/irida_table_view.hpp"
#include <cstdint>
#include <vector>
class DebugController;

class RegistersPanel : public IridaTableView {
    Q_OBJECT
  public:
    explicit RegistersPanel(DebugController* controller, QWidget* parent = nullptr);
  public slots:
    void refresh();

  private:
    DebugController* controller_;
    std::vector<uint64_t> prev_values_;
};
