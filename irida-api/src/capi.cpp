// SPDX-License-Identifier: BUSL-1.1
#include "backend.hpp"
#include "irida/irida.h"
#include <new>

extern "C" {

IridaSession* irida_session_create(const IridaBackendVTable* vt, void* ctx) {
    if (!vt)
        return nullptr;
    auto* s = new (std::nothrow) IridaSession{*vt, ctx};
    return s;
}

void irida_session_destroy(IridaSession* s) {
    delete s;
}

size_t irida_registers(IridaSession* s, const IridaRegister** out) {
    if (!out)
        return 0;
    if (!s || !s->vt.registers) {
        *out = nullptr;
        return 0;
    }
    return s->vt.registers(s->ctx, out);
}

size_t irida_modules(IridaSession* s, const IridaModule** out) {
    if (!out)
        return 0;
    if (!s || !s->vt.modules) {
        *out = nullptr;
        return 0;
    }
    return s->vt.modules(s->ctx, out);
}

size_t irida_maps(IridaSession* s, const IridaMemMap** out) {
    if (!out)
        return 0;
    if (!s || !s->vt.maps) {
        *out = nullptr;
        return 0;
    }
    return s->vt.maps(s->ctx, out);
}

size_t irida_threads(IridaSession* s, const IridaThread** out) {
    if (!out)
        return 0;
    if (!s || !s->vt.threads) {
        *out = nullptr;
        return 0;
    }
    return s->vt.threads(s->ctx, out);
}

size_t irida_disasm(IridaSession* s, uint64_t addr, size_t count, const IridaInsnRow** out) {
    if (!out)
        return 0;
    if (!s || !s->vt.disasm) {
        *out = nullptr;
        return 0;
    }
    return s->vt.disasm(s->ctx, addr, count, out);
}

IridaRunState irida_run_state(IridaSession* s) {
    if (!s || !s->vt.run_state)
        return IRIDA_STOPPED;
    return s->vt.run_state(s->ctx);
}

uint64_t irida_pc(IridaSession* s) {
    if (!s || !s->vt.pc)
        return 0;
    return s->vt.pc(s->ctx);
}

uint64_t irida_state_epoch(IridaSession* s) {
    if (!s || !s->vt.state_epoch)
        return 0;
    return s->vt.state_epoch(s->ctx);
}

void irida_step_into(IridaSession* s) {
    if (s && s->vt.step_into)
        s->vt.step_into(s->ctx);
}

void irida_step_over(IridaSession* s) {
    if (s && s->vt.step_over)
        s->vt.step_over(s->ctx);
}

void irida_step_out(IridaSession* s) {
    if (s && s->vt.step_out)
        s->vt.step_out(s->ctx);
}

void irida_continue(IridaSession* s) {
    if (s && s->vt.cont)
        s->vt.cont(s->ctx);
}

void irida_break(IridaSession* s) {
    if (s && s->vt.brk)
        s->vt.brk(s->ctx);
}

void irida_restart(IridaSession* s) {
    if (s && s->vt.restart)
        s->vt.restart(s->ctx);
}

void irida_stop(IridaSession* s) {
    if (s && s->vt.stop)
        s->vt.stop(s->ctx);
}

size_t irida_read_memory(IridaSession* s, uint64_t addr, uint8_t* buf, size_t len) {
    if (!s || !s->vt.read_memory || !buf)
        return 0;
    return s->vt.read_memory(s->ctx, addr, buf, len);
}

size_t irida_breakpoints(IridaSession* s, const IridaBreakpoint** out) {
    if (!out)
        return 0;
    if (!s || !s->vt.breakpoints) {
        *out = nullptr;
        return 0;
    }
    return s->vt.breakpoints(s->ctx, out);
}

void irida_bp_toggle(IridaSession* s, uint64_t addr) {
    if (s && s->vt.bp_toggle)
        s->vt.bp_toggle(s->ctx, addr);
}

void irida_bp_set_enabled(IridaSession* s, uint64_t addr, int enabled) {
    if (s && s->vt.bp_set_enabled)
        s->vt.bp_set_enabled(s->ctx, addr, enabled);
}

} // extern "C"
