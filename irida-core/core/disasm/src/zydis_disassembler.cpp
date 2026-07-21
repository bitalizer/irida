// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// The ONLY file in this project that includes <Zydis/Zydis.h>. Everything
// above this file (the Disassembler interface, Instruction/Operand types)
// is Zydis-free by design so the arch backend is swappable later.
#include "irida/disasm/disassembler.hpp"
#include <Zydis/Zydis.h>

namespace irida::disasm {

namespace {

std::string register_name(ZydisRegister reg) {
    const char* name = ZydisRegisterGetString(reg);
    return name != nullptr ? std::string(name) : std::string();
}

Operand map_operand(const ZydisDecodedOperand& op) {
    Operand out;
    switch (op.type) {
    case ZYDIS_OPERAND_TYPE_REGISTER:
        out.kind = OperandKind::Register;
        out.reg = register_name(op.reg.value);
        break;
    case ZYDIS_OPERAND_TYPE_MEMORY:
        out.kind = OperandKind::Memory;
        if (op.mem.base != ZYDIS_REGISTER_NONE) {
            out.mem.base = register_name(op.mem.base);
        }
        if (op.mem.index != ZYDIS_REGISTER_NONE) {
            out.mem.index = register_name(op.mem.index);
        }
        out.mem.scale = op.mem.index != ZYDIS_REGISTER_NONE ? op.mem.scale : 1;
        out.mem.disp = op.mem.disp.has_displacement ? op.mem.disp.value : 0;
        // Zydis has no dedicated rip-relative flag; a memory operand is
        // rip-relative iff its base register is RIP (x86-64 only).
        out.mem.rip_relative = (op.mem.base == ZYDIS_REGISTER_RIP);
        break;
    case ZYDIS_OPERAND_TYPE_IMMEDIATE:
        out.kind = OperandKind::Immediate;
        out.imm = op.imm.value.u;
        break;
    case ZYDIS_OPERAND_TYPE_POINTER:
        out.kind = OperandKind::Pointer;
        out.imm = (static_cast<uint64_t>(op.ptr.segment) << 32) | op.ptr.offset;
        break;
    default:
        out.kind = OperandKind::Unknown;
        break;
    }
    return out;
}

// Map a Zydis instruction to our control-flow class. Uses meta.category, which
// groups mnemonics regardless of the specific condition (all Jcc share
// COND_BR, jmp is UNCOND_BR, call/ret/syscall have dedicated categories).
Flow classify_flow(const ZydisDecodedInstruction& insn) {
    switch (insn.meta.category) {
    case ZYDIS_CATEGORY_CALL:
        return Flow::Call;
    case ZYDIS_CATEGORY_RET:
        return Flow::Return;
    case ZYDIS_CATEGORY_UNCOND_BR:
        return Flow::Jump;
    case ZYDIS_CATEGORY_COND_BR:
        return Flow::CondJump;
    default:
        return Flow::Sequential;
    }
}

class ZydisDisassembler final : public Disassembler {
  public:
    ZydisDisassembler() {
        ZydisDecoderInit(&decoder_, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        ZydisFormatterInit(&formatter_, ZYDIS_FORMATTER_STYLE_INTEL);
    }

    irida::base::Result<Instruction> decode_one(std::span<const std::byte> bytes,
                                                uint64_t address) override {
        ZydisDecodedInstruction insn;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
        const auto* data = reinterpret_cast<const ZyanU8*>(bytes.data());
        if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder_, data, bytes.size(), &insn, operands))) {
            return irida::base::Result<Instruction>::err("failed to decode instruction");
        }

        Instruction out;
        out.address = address;
        out.length = insn.length;
        const char* mnemonic = ZydisMnemonicGetString(insn.mnemonic);
        out.mnemonic = mnemonic != nullptr ? std::string(mnemonic) : std::string();

        char text_buf[256];
        if (ZYAN_SUCCESS(ZydisFormatterFormatInstruction(&formatter_, &insn, operands,
                                                         insn.operand_count_visible, text_buf,
                                                         sizeof(text_buf), address, ZYAN_NULL))) {
            out.text = text_buf;
        }

        out.operands.reserve(insn.operand_count_visible);
        for (ZyanU8 i = 0; i < insn.operand_count_visible; ++i) {
            out.operands.push_back(map_operand(operands[i]));
        }

        out.flow = classify_flow(insn);
        if (out.flow == Flow::Jump || out.flow == Flow::CondJump || out.flow == Flow::Call) {
            for (ZyanU8 i = 0; i < insn.operand_count_visible; ++i) {
                if (operands[i].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
                    ZyanU64 target = 0;
                    if (ZYAN_SUCCESS(
                            ZydisCalcAbsoluteAddress(&insn, &operands[i], address, &target))) {
                        out.branch_target = target;
                    }
                    break;
                }
            }
        }

        return irida::base::Result<Instruction>::ok(std::move(out));
    }

    std::vector<Instruction> decode_linear(std::span<const std::byte> bytes, uint64_t address,
                                           size_t max) override {
        std::vector<Instruction> out;
        size_t offset = 0;
        while (out.size() < max && offset < bytes.size()) {
            auto result = decode_one(bytes.subspan(offset), address + offset);
            if (!result.has_value()) {
                break;
            }
            Instruction insn = std::move(result).value();
            offset += insn.length;
            out.push_back(std::move(insn));
        }
        return out;
    }

  private:
    ZydisDecoder decoder_{};
    ZydisFormatter formatter_{};
};

} // namespace

std::unique_ptr<Disassembler> make_x86_64_disassembler() {
    return std::make_unique<ZydisDisassembler>();
}

} // namespace irida::disasm
