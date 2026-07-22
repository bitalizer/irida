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
    SessionKind kind() const {
        return session_kind_;
    }
    // True when a live process backs the session (registers, stepping, live
    // values are meaningful). False for a static file session.
    bool isLive() const {
        return session_kind_ == SessionKind::Native;
    }

    // Replaces the active session, destroying the previous one with the
    // destructor matching how it was created.
    void setSession(IridaSession* s, SessionKind kind);

    // True while a background analysis pass is running.
    bool analyzing() const {
        return analyzing_;
    }

    // Runs irida_analyze() on a worker thread and emits analysisFinished() when
    // done (analysisStarted() first). Analysis-dependent views refresh on
    // analysisFinished rather than calling irida_analyze on the UI thread.
    void runAnalysis();

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
    void analysisStarted();
    void analysisFinished();
    // Emitted when the session is swapped for one of a different kind (e.g. a
    // static file session replaced by a live process on attach), so the UI can
    // show or hide debug-only surfaces.
    void sessionKindChanged();

  private:
    void afterOp(uint64_t epoch_before);
    void destroySession();
    IridaSession* session_;
    SessionKind session_kind_ = SessionKind::Mock;
    bool analyzing_ = false;
};
