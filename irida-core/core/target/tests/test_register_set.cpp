// SPDX-License-Identifier: BUSL-1.1
#include "irida/target/register_set.hpp"
#include <cassert>
#include <cstddef>
#include <vector>

using irida::target::RegisterSet;

namespace {

std::vector<std::byte> make_g_block() {
    // 17 little-endian 64-bit slots (rax..r15, rip) + one 32-bit eflags slot.
    // Slot i (64-bit) gets value (i + 1) repeated as its low byte pattern so
    // decoding is easy to verify: slot 0 -> 0x01, slot 1 -> 0x02, etc.
    std::vector<std::byte> block;
    for (int slot = 0; slot < 17; ++slot) {
        for (int b = 0; b < 8; ++b) {
            // Little-endian: least-significant byte first. Encode the slot
            // index in the low byte, zero elsewhere, so value == slot.
            block.push_back(b == 0 ? std::byte{static_cast<unsigned char>(slot)} : std::byte{0});
        }
    }
    // eflags: 32-bit slot, value 0x00000246 (typical flags), little-endian.
    block.push_back(std::byte{0x46});
    block.push_back(std::byte{0x02});
    block.push_back(std::byte{0x00});
    block.push_back(std::byte{0x00});
    return block;
}

} // namespace

int main() {
    auto block = make_g_block();
    auto decoded = RegisterSet::decode(block);
    assert(decoded.has_value());
    const RegisterSet& regs = decoded.value();

    // Order: rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8-r15, rip (17 slots), then eflags.
    auto rax = regs.get("rax");
    assert(rax.has_value() && *rax == 0);
    auto rbx = regs.get("rbx");
    assert(rbx.has_value() && *rbx == 1);
    auto rsp = regs.get("rsp");
    assert(rsp.has_value() && *rsp == 7);
    auto r8 = regs.get("r8");
    assert(r8.has_value() && *r8 == 8);
    auto r15 = regs.get("r15");
    assert(r15.has_value() && *r15 == 15);
    auto rip = regs.get("rip");
    assert(rip.has_value() && *rip == 16);
    auto eflags = regs.get("eflags");
    assert(eflags.has_value() && *eflags == 0x246);

    auto missing = regs.get("not_a_register");
    assert(!missing.has_value());

    // all() should contain at least the 18 known registers.
    assert(regs.all().size() >= 18);

    // Short block: decode what's present, missing regs simply absent.
    std::vector<std::byte> short_block(16, std::byte{0}); // only rax, rbx present (2 slots)
    short_block[0] = std::byte{0xAA};
    auto decoded_short = RegisterSet::decode(short_block);
    assert(decoded_short.has_value());
    const RegisterSet& regs_short = decoded_short.value();
    auto rax_short = regs_short.get("rax");
    assert(rax_short.has_value() && *rax_short == 0xAA);
    auto rcx_short = regs_short.get("rcx");
    assert(!rcx_short.has_value());

    return 0;
}
