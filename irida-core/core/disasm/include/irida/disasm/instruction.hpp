// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace irida::disasm {

enum class OperandKind { Register, Memory, Immediate, Pointer, Unknown };

struct MemoryOperand { // [base + index*scale + disp]
    std::string base;  // register name, "" if none (e.g. "rax")
    std::string index; // "" if none
    uint8_t scale = 1;
    int64_t disp = 0;
    bool rip_relative = false; // Zydis reports this; disp is then vs next-insn
};

struct Operand {
    OperandKind kind = OperandKind::Unknown;
    std::string reg;   // Register: reg name
    MemoryOperand mem; // Memory: the components (this is what M5 evaluates)
    uint64_t imm = 0;  // Immediate/Pointer: the value
    std::string text;  // formatted text of just this operand (fallback/display)
};

struct Instruction {
    uint64_t address = 0; // where it was decoded
    uint32_t length = 0;  // bytes consumed
    std::string mnemonic; // "mov", "lea", "call", ...
    std::string text;     // full formatted instruction text ("mov rcx, [rax+8]")
    std::vector<Operand> operands;
};

} // namespace irida::disasm
