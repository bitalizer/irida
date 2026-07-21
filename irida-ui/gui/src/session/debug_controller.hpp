// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/irida.h"
#include <QObject>
#include <cstdint>

// Sole coordinator of the debug session. Panels never call ABI control ops
// directly; they invoke these slots and listen to stateChanged(). When the real
// engine replaces the mock, only this class changes (worker thread + queued
// stateChanged()) — panels are untouched.
// How a session was created, so it is destroyed with the matching ABI call.
enum class SessionKind { Mock, Static, Native };

class DebugController : public QObject {
    Q_OBJECT
  public:
    explicit DebugController(IridaSession* session, QObject* parent = nullptr,
                             SessionKind kind = SessionKind::Mock);
    ~DebugController() override;
    IridaSession* session() const {
        return session_;
    }

    // Replaces the active session, destroying the previous one with the
    // destructor matching how it was created.
    void setSession(IridaSession* s, SessionKind kind);

  public slots:
    // Signals every connected view to reload from the current session. Used to
    // populate panels once after the window is shown, off the construction path.
    void refreshViews();
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
    void destroySession();
    IridaSession* session_;
    SessionKind session_kind_ = SessionKind::Mock;
};
