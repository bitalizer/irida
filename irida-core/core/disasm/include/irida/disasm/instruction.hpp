// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace irida::disasm {

enum class OperandKind { Register, Memory, Immediate, Pointer, Unknown };

// Control-flow class of an instruction. Drives recursive-descent analysis:
//   Jump      -> unconditional transfer; block ends, follow branch_target
//   CondJump  -> conditional; block ends with two edges (branch_target + next)
//   Call      -> subroutine call; branch_target is a new function entry, flow
//                falls through to the next instruction
//   Return    -> block ends, path terminates
//   Sequential-> no transfer; decoding continues into the next instruction
enum class Flow { Sequential, Jump, CondJump, Call, Return };

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
    Flow flow = Flow::Sequential;
    // Resolved absolute branch/call target for direct transfers (Jump/CondJump/
    // Call with an immediate operand). 0 when the target is indirect (register
    // or memory) or the instruction does not transfer control.
    uint64_t branch_target = 0;
};

} // namespace irida::disasm
