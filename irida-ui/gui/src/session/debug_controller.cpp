// SPDX-License-Identifier: BUSL-1.1
#include "session/debug_controller.hpp"

DebugController::DebugController(IridaSession* session, QObject* parent)
    : QObject(parent), session_(session), session_is_native_(false) {}

DebugController::~DebugController() {
    destroySession();
}

void DebugController::destroySession() {
    if (!session_)
        return;
    if (session_is_native_)
        irida_session_destroy_native(session_);
    else
        irida_session_destroy(session_);
    session_ = nullptr;
}

void DebugController::setSession(IridaSession* s, bool is_native) {
    destroySession();
    session_ = s;
    session_is_native_ = is_native;
    emit stateChanged();
}

void DebugController::afterOp(uint64_t epoch_before) {
    if (irida_state_epoch(session_) != epoch_before)
        emit stateChanged();
}

void DebugController::stepInto() {
    uint64_t e = irida_state_epoch(session_);
    irida_step_into(session_);
    afterOp(e);
}
void DebugController::stepOver() {
    uint64_t e = irida_state_epoch(session_);
    irida_step_over(session_);
    afterOp(e);
}
void DebugController::stepOut() {
    uint64_t e = irida_state_epoch(session_);
    irida_step_out(session_);
    afterOp(e);
}
void DebugController::run() {
    uint64_t e = irida_state_epoch(session_);
    irida_continue(session_);
    afterOp(e);
}
void DebugController::breakExec() {
    uint64_t e = irida_state_epoch(session_);
    irida_break(session_);
    afterOp(e);
}
void DebugController::restart() {
    uint64_t e = irida_state_epoch(session_);
    irida_restart(session_);
    afterOp(e);
}
void DebugController::stop() {
    uint64_t e = irida_state_epoch(session_);
    irida_stop(session_);
    afterOp(e);
}
void DebugController::toggleBreakpoint(uint64_t addr) {
    irida_bp_toggle(session_, addr);
    emit stateChanged();
}
void DebugController::setBreakpointEnabled(uint64_t addr, bool on) {
    irida_bp_set_enabled(session_, addr, on ? 1 : 0);
    emit stateChanged();
}
void DebugController::navigateTo(uint64_t addr) {
    emit navigationRequested(addr);
}
