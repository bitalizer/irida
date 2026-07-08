// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/irida.h"
#include <QObject>
#include <cstdint>

// Sole coordinator of the debug session. Panels never call ABI control ops
// directly; they invoke these slots and listen to stateChanged(). When the real
// engine replaces the mock, only this class changes (worker thread + queued
// stateChanged()) — panels are untouched.
class DebugController : public QObject {
    Q_OBJECT
  public:
    explicit DebugController(IridaSession* session, QObject* parent = nullptr);
    IridaSession* session() const {
        return session_;
    }

  public slots:
    void stepInto();
    void stepOver();
    void stepOut();
    void run();
    void breakExec();
    void restart();
    void stop();
    void toggleBreakpoint(uint64_t addr);
    void setBreakpointEnabled(uint64_t addr, bool on);
    void navigateTo(uint64_t addr);

  signals:
    void stateChanged();
    void navigationRequested(uint64_t addr);

  private:
    void afterOp(uint64_t epoch_before);
    IridaSession* session_;
};
