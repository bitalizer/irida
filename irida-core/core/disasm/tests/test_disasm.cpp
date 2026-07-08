// SPDX-License-Identifier: BUSL-1.1
#include "irida/disasm/disassembler.hpp"
#include <cassert>
#include <cstddef>
#include <vector>

using irida::disasm::Disassembler;
using irida::disasm::Instruction;
using irida::disasm::OperandKind;

namespace {

std::span<const std::byte> as_bytes(const std::vector<uint8_t>& v) {
    return std::span<const std::byte>(reinterpret_cast<const std::byte*>(v.data()), v.size());
}

} // namespace

int main() {
    auto disasm = irida::disasm::make_x86_64_disassembler();
    assert(disasm != nullptr);

    // `55` -> push rbp (1 byte, mnemonic "push", 1 reg operand "rbp").
    {
        std::vector<uint8_t> bytes{0x55};
        auto result = disasm->decode_one(as_bytes(bytes), 0x1000);
        assert(result.has_value());
        const Instruction& insn = result.value();
        assert(insn.length == 1);
        assert(insn.address == 0x1000);
        assert(insn.mnemonic == "push");
        assert(insn.operands.size() == 1);
        assert(insn.operands[0].kind == OperandKind::Register);
        assert(insn.operands[0].reg == "rbp");
    }

    // `48 8B 48 08` -> mov rcx, [rax+8] (4 bytes; op0 reg "rcx"; op1 memory base="rax", disp=8).
    {
        std::vector<uint8_t> bytes{0x48, 0x8B, 0x48, 0x08};
        auto result = disasm->decode_one(as_bytes(bytes), 0x2000);
        assert(result.has_value());
        const Instruction& insn = result.value();
        assert(insn.length == 4);
        assert(insn.mnemonic == "mov");
        assert(insn.operands.size() == 2);
        assert(insn.operands[0].kind == OperandKind::Register);
        assert(insn.operands[0].reg == "rcx");
        assert(insn.operands[1].kind == OperandKind::Memory);
        assert(insn.operands[1].mem.base == "rax");
        assert(insn.operands[1].mem.disp == 8);
        assert(insn.operands[1].mem.scale == 1);
        assert(insn.operands[1].mem.index.empty());
        assert(!insn.operands[1].mem.rip_relative);
    }

    // `48 8D 05 xx xx xx xx` -> lea rax, [rip+disp] -> memory operand rip_relative=true.
    {
        std::vector<uint8_t> bytes{0x48, 0x8D, 0x05, 0x10, 0x00, 0x00, 0x00};
        auto result = disasm->decode_one(as_bytes(bytes), 0x3000);
        assert(result.has_value());
        const Instruction& insn = result.value();
        assert(insn.length == 7);
        assert(insn.mnemonic == "lea");
        assert(insn.operands.size() == 2);
        assert(insn.operands[0].kind == OperandKind::Register);
        assert(insn.operands[0].reg == "rax");
        assert(insn.operands[1].kind == OperandKind::Memory);
        assert(insn.operands[1].mem.rip_relative);
        assert(insn.operands[1].mem.disp == 0x10);
    }

    // `E8 00 00 00 00` -> call with immediate/relative operand.
    {
        std::vector<uint8_t> bytes{0xE8, 0x00, 0x00, 0x00, 0x00};
        auto result = disasm->decode_one(as_bytes(bytes), 0x4000);
        assert(result.has_value());
        const Instruction& insn = result.value();
        assert(insn.length == 5);
        assert(insn.mnemonic == "call");
        assert(insn.operands.size() == 1);
        assert(insn.operands[0].kind == OperandKind::Immediate);
    }

    // decode_linear over a multi-instruction buffer -> correct count + addresses advancing.
    {
        std::vector<uint8_t> bytes{
            0x55,                         // push rbp
            0x48, 0x8B, 0x48, 0x08,       // mov rcx, [rax+8]
            0xE8, 0x00, 0x00, 0x00, 0x00, // call
        };
        auto insns = disasm->decode_linear(as_bytes(bytes), 0x5000, 10);
        assert(insns.size() == 3);
        assert(insns[0].address == 0x5000);
        assert(insns[0].length == 1);
        assert(insns[1].address == 0x5001);
        assert(insns[1].length == 4);
        assert(insns[2].address == 0x5005);
        assert(insns[2].length == 5);
    }

    // decode_linear respects `max`.
    {
        std::vector<uint8_t> bytes{0x55, 0x55, 0x55, 0x55};
        auto insns = disasm->decode_linear(as_bytes(bytes), 0x6000, 2);
        assert(insns.size() == 2);
    }

    // Undecodable/truncated buffer -> decode_one returns err; decode_linear stops gracefully.
    {
        std::vector<uint8_t> bytes{}; // empty buffer
        auto result = disasm->decode_one(as_bytes(bytes), 0x7000);
        assert(!result.has_value());

        auto insns = disasm->decode_linear(as_bytes(bytes), 0x7000, 10);
        assert(insns.empty());
    }
    {
        // Truncated mov rcx,[rax+8]: only 2 of 4 bytes present.
        std::vector<uint8_t> bytes{0x48, 0x8B};
        auto result = disasm->decode_one(as_bytes(bytes), 0x8000);
        assert(!result.has_value());
    }
    {
        // Valid insn followed by truncated bytes: decode_linear stops after the valid one.
        std::vector<uint8_t> bytes{0x55, 0x48, 0x8B};
        auto insns = disasm->decode_linear(as_bytes(bytes), 0x9000, 10);
        assert(insns.size() == 1);
        assert(insns[0].mnemonic == "push");
    }

    return 0;
}
