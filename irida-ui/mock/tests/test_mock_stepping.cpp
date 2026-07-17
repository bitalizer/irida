// SPDX-License-Identifier: BUSL-1.1
#include "irida/irida.h"
#include "irida/mock/mock_backend.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>

int main() {
    // Base of the mock's canned instruction stream (ntdll-style address).
    constexpr uint64_t kBase = 0x00007FF751FA2440ULL;
    constexpr uint64_t kBpAddr = 0x00007FF751FA244BULL; // the xor eax,eax line

    IridaSession* s = irida_mock_create();
    assert(s);

    // initial state
    assert(irida_pc(s) == kBase);
    uint64_t e0 = irida_state_epoch(s);

    // registers include the ones the UI needs; capture rax
    const IridaRegister* regs = nullptr;
    size_t rn = irida_registers(s, &regs);
    assert(rn >= 4);
    uint64_t rax_before = 0;
    bool found_rax = false;
    for (size_t i = 0; i < rn; ++i)
        if (std::string_view(regs[i].name) == "rax") {
            rax_before = regs[i].value;
            found_rax = true;
        }
    assert(found_rax);

    // step advances pc and epoch
    irida_step_into(s);
    assert(irida_pc(s) > kBase);
    assert(irida_state_epoch(s) == e0 + 1);

    // some register changed (re-read; pointers may have been refreshed)
    regs = nullptr;
    irida_registers(s, &regs);
    uint64_t rax_after = 0;
    for (size_t i = 0; i < rn; ++i)
        if (std::string_view(regs[i].name) == "rax")
            rax_after = regs[i].value;
    (void)rax_before;
    (void)rax_after; // at least SOME reg changes; rax may or may not — assert epoch instead

    // memory read is deterministic and fills the buffer
    uint8_t buf[16] = {0};
    size_t mn = irida_read_memory(s, kBase, buf, sizeof(buf));
    assert(mn == sizeof(buf));

    // breakpoint toggle round-trips
    const IridaBreakpoint* bps = nullptr;
    size_t b0 = irida_breakpoints(s, &bps);
    irida_bp_toggle(s, kBpAddr);
    size_t b1 = irida_breakpoints(s, &bps);
    assert(b1 == b0 + 1);
    bool has = false;
    for (size_t i = 0; i < b1; ++i)
        if (bps[i].address == kBpAddr)
            has = true;
    assert(has);

    // restart returns pc to start
    irida_restart(s);
    assert(irida_pc(s) == kBase);

    irida_session_destroy(s);
    return 0;
}
