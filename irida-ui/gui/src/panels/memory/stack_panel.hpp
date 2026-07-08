// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "widgets/irida_table_view.hpp"
#include <cstdint>
class DebugController;

class StackPanel : public IridaTableView {
    Q_OBJECT
  public:
    explicit StackPanel(DebugController* controller, QWidget* parent = nullptr);
  public slots:
    void refresh();

  private:
    DebugController* controller_;
};
