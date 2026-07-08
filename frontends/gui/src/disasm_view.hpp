// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/irida.h"
#include <QTableWidget>

class DisasmView : public QTableWidget {
    Q_OBJECT
  public:
    explicit DisasmView(QWidget* parent = nullptr);
    // Pull `count` rows starting at `addr` from the session and display them.
    void load(IridaSession* session, uint64_t addr, size_t count);
};
