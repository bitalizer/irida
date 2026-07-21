// SPDX-License-Identifier: BUSL-1.1
// Exercises the addr->name resolver helpers in capi_target.cpp directly.
// They live in that file's anonymous namespace (no ABI surface of their
// own), so this test compiles the .cpp as its own translation unit rather
// than linking against irida_capi.
#include "../src/capi_target.cpp"
#include <cassert>

using irida::binfmt::BinaryInfo;
using irida::binfmt::Export;
using irida::binfmt::Import;
using irida::binfmt::Symbol;
using irida::disasm::Instruction;
using irida::disasm::Operand;
using irida::disasm::OperandKind;

namespace {

BinaryInfo make_bin() {
    BinaryInfo bin;
    bin.image_base = 0x140000000ULL;
    bin.symbols.push_back(Symbol{"internal_func", 0x140001000ULL, "function"});
    bin.exports.push_back(Export{"DoTheThing", 0x140002000ULL, 1});
    bin.imports.push_back(Import{"CreateFileW", "kernel32.dll", 0x140003000ULL});
    return bin;
}

} // namespace

int main() {
    BinaryInfo bin = make_bin();

    // Same base: file addresses used as-is (mock-like, no rebase needed).
    auto map = build_symbol_map(bin, bin.image_base, bin.image_base);
    assert(map.at(0x140001000ULL) == "internal_func");
    assert(map.at(0x140002000ULL) == "sym.DoTheThing");
    assert(map.at(0x140003000ULL) == "sym.imp.CreateFileW");

    // Different runtime base: every file addr shifts by the same delta.
    uint64_t runtime_base = 0x7ff600000000ULL;
    auto rebased = build_symbol_map(bin, runtime_base, bin.image_base);
    assert(rebased.at(runtime_base + 0x1000) == "internal_func");
    assert(rebased.at(runtime_base + 0x2000) == "sym.DoTheThing");
    assert(rebased.at(runtime_base + 0x3000) == "sym.imp.CreateFileW");
    assert(rebased.find(0x140003000ULL) == rebased.end());

    // Collision: import wins over export wins over symbol at the same addr.
    BinaryInfo collide;
    collide.image_base = 0;
    collide.symbols.push_back(Symbol{"as_symbol", 0x1000, "function"});
    collide.exports.push_back(Export{"as_export", 0x1000, 1});
    auto collide_map = build_symbol_map(collide, 0, 0);
    assert(collide_map.at(0x1000) == "sym.as_export");
    collide.imports.push_back(Import{"as_import", "lib.dll", 0x1000});
    collide_map = build_symbol_map(collide, 0, 0);
    assert(collide_map.at(0x1000) == "sym.imp.as_import");

    // operand_target: immediate/pointer operand wins.
    {
        Instruction insn;
        insn.address = 0x1000;
        insn.length = 5;
        Operand op;
        op.kind = OperandKind::Immediate;
        op.imm = 0x1400;
        insn.operands.push_back(op);
        auto tgt = operand_target(insn);
        assert(tgt.has_value() && tgt.value() == 0x1400);
    }

    // operand_target: rip-relative memory operand computes address+length+disp.
    {
        Instruction insn;
        insn.address = 0x2000;
        insn.length = 7;
        Operand op;
        op.kind = OperandKind::Memory;
        op.mem.rip_relative = true;
        op.mem.disp = 0x100;
        insn.operands.push_back(op);
        auto tgt = operand_target(insn);
        assert(tgt.has_value() && tgt.value() == 0x2000 + 7 + 0x100);
    }

    // operand_target: no immediate/pointer/rip-relative operand -> nullopt.
    {
        Instruction insn;
        insn.address = 0x3000;
        insn.length = 2;
        Operand op;
        op.kind = OperandKind::Register;
        op.reg = "rax";
        insn.operands.push_back(op);
        assert(!operand_target(insn).has_value());
    }

    return 0;
}
