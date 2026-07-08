// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include "irida/base/result.hpp"
#include "irida/disasm/instruction.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace irida::disasm {

class Disassembler {
  public:
    virtual ~Disassembler() = default;

    // Decode ONE instruction at `address` from `bytes` (bytes[0] is the insn start).
    // Returns the structured Instruction, or an error if bytes don't form a valid insn.
    virtual irida::base::Result<Instruction> decode_one(std::span<const std::byte> bytes,
                                                        uint64_t address) = 0;

    // Decode up to `max` instructions linearly from `bytes` starting at `address`.
    // Stops at end of buffer or first undecodable byte. Never throws.
    virtual std::vector<Instruction> decode_linear(std::span<const std::byte> bytes,
                                                   uint64_t address, size_t max) = 0;
};

// Factory for the x86-64 Zydis-backed disassembler.
std::unique_ptr<Disassembler> make_x86_64_disassembler();

} // namespace irida::disasm
