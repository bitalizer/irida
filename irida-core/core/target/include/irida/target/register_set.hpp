// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/base/result.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace irida::target {

// The current register file, decoded from a raw gdb-remote `g`-packet block.
//
// x86-64 register layout is hardcoded: the general-purpose registers plus
// rip (seventeen 64-bit little-endian slots, in QEMU's default g-block
// order), followed by a 32-bit eflags slot. Full FPU/SSE/segment decode is
// deferred to a later milestone.
class RegisterSet {
  public:
    static irida::base::Result<RegisterSet> decode(std::span<const std::byte> g_block);

    std::optional<uint64_t> get(std::string_view name) const;
    const std::vector<std::pair<std::string, uint64_t>>& all() const;

  private:
    RegisterSet() = default;

    std::vector<std::pair<std::string, uint64_t>> regs_;
};

} // namespace irida::target
