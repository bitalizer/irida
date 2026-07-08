// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/irida.h"
#include <QMainWindow>
class DisasmView;

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(IridaSession* session, QWidget* parent = nullptr);

  private:
    DisasmView* disasm_;
};
