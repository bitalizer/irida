// SPDX-License-Identifier: BUSL-1.1
#include "irida/target/register_set.hpp"
#include <algorithm>
#include <array>

namespace irida::target {

using irida::base::Result;

namespace {

// QEMU's default x86-64 g-block order: 17 64-bit GP/rip slots, then a
// 32-bit eflags slot. Segment registers and beyond are deferred.
constexpr std::array<std::string_view, 17> kQwordRegs = {
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "r8",
    "r9",  "r10", "r11", "r12", "r13", "r14", "r15", "rip",
};
constexpr std::string_view kEflagsName = "eflags";
constexpr size_t kQwordSize = 8;
constexpr size_t kDwordSize = 4;

uint64_t decode_le(std::span<const std::byte> bytes) {
    uint64_t value = 0;
    for (size_t i = 0; i < bytes.size(); ++i) {
        value |= static_cast<uint64_t>(std::to_integer<unsigned char>(bytes[i])) << (8 * i);
    }
    return value;
}

} // namespace

Result<RegisterSet> RegisterSet::decode(std::span<const std::byte> g_block) {
    RegisterSet regs;
    size_t offset = 0;

    for (const auto& name : kQwordRegs) {
        if (offset >= g_block.size())
            break;
        size_t take = std::min(kQwordSize, g_block.size() - offset);
        regs.regs_.emplace_back(std::string(name), decode_le(g_block.subspan(offset, take)));
        offset += take;
    }

    if (offset < g_block.size()) {
        size_t take = std::min(kDwordSize, g_block.size() - offset);
        regs.regs_.emplace_back(std::string(kEflagsName), decode_le(g_block.subspan(offset, take)));
        offset += take;
    }

    return Result<RegisterSet>::ok(std::move(regs));
}

std::optional<uint64_t> RegisterSet::get(std::string_view name) const {
    for (const auto& [reg_name, value] : regs_) {
        if (reg_name == name)
            return value;
    }
    return std::nullopt;
}

const std::vector<std::pair<std::string, uint64_t>>& RegisterSet::all() const {
    return regs_;
}

} // namespace irida::target
