// SPDX-License-Identifier: BUSL-1.1
#include "irida/irida.h"
#include <cassert>
#include <cstdint>
#include <cstring>

namespace {
struct Ctx {
    uint64_t pc = 0x2000;
    uint64_t epoch = 7;
    int step_into_calls = 0;
    IridaBreakpoint bps[2] = {{0x1000, 1, IRIDA_BP_SOFTWARE}, {0x1004, 0, IRIDA_BP_HARDWARE}};
};
IridaRunState rs(void*) {
    return IRIDA_STOPPED;
}
uint64_t pc(void* c) {
    return static_cast<Ctx*>(c)->pc;
}
uint64_t epoch(void* c) {
    return static_cast<Ctx*>(c)->epoch;
}
void step_into(void* c) {
    auto* x = static_cast<Ctx*>(c);
    x->pc += 4;
    x->epoch++;
    x->step_into_calls++;
}
void noop(void*) {}
size_t read_mem(void* /*c*/, uint64_t /*addr*/, uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; ++i)
        buf[i] = static_cast<uint8_t>(i);
    return len;
}
size_t bps(void* c, const IridaBreakpoint** out) {
    *out = static_cast<Ctx*>(c)->bps;
    return 2;
}
void bp_toggle(void*, uint64_t) {}
void bp_set_enabled(void*, uint64_t, int) {}

// existing three left null here (we only test the new surface)
IridaBackendVTable make_vt() {
    IridaBackendVTable vt;
    std::memset(&vt, 0, sizeof(vt));
    vt.run_state = rs;
    vt.pc = pc;
    vt.state_epoch = epoch;
    vt.step_into = step_into;
    vt.step_over = noop;
    vt.step_out = noop;
    vt.cont = noop;
    vt.brk = noop;
    vt.restart = noop;
    vt.stop = noop;
    vt.read_memory = read_mem;
    vt.breakpoints = bps;
    vt.bp_toggle = bp_toggle;
    vt.bp_set_enabled = bp_set_enabled;
    return vt;
}
} // namespace

int main() {
    Ctx ctx;
    IridaBackendVTable vt = make_vt();
    IridaSession* s = irida_session_create(&vt, &ctx);
    assert(s);

    assert(irida_run_state(s) == IRIDA_STOPPED);
    assert(irida_pc(s) == 0x2000);
    assert(irida_state_epoch(s) == 7);

    irida_step_into(s);
    assert(ctx.step_into_calls == 1);
    assert(irida_pc(s) == 0x2004);
    assert(irida_state_epoch(s) == 8);

    uint8_t buf[8] = {0};
    size_t n = irida_read_memory(s, 0x1000, buf, sizeof(buf));
    assert(n == 8);
    assert(buf[3] == 3);

    const IridaBreakpoint* out = nullptr;
    size_t bn = irida_breakpoints(s, &out);
    assert(bn == 2);
    assert(out[0].address == 0x1000 && out[0].enabled == 1);
    assert(out[1].type == IRIDA_BP_HARDWARE);

    // null-guards: no crash, sane returns
    assert(irida_pc(nullptr) == 0);
    assert(irida_read_memory(nullptr, 0, buf, sizeof(buf)) == 0);
    const IridaBreakpoint* o2 = nullptr;
    assert(irida_breakpoints(nullptr, &o2) == 0);

    irida_session_destroy(s);
    return 0;
}
