// SPDX-License-Identifier: BUSL-1.1
#include "irida/analysis/analysis.hpp"
#include "irida/disasm/disassembler.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

using irida::analysis::AnalysisResult;
using irida::analysis::Analyzer;
using irida::analysis::BasicBlock;
using irida::analysis::Function;
using irida::analysis::XrefKind;

namespace {

// A flat program image keyed by base address. The reader serves bytes from
// whichever planted region contains the requested address.
struct Image {
    std::map<uint64_t, std::vector<uint8_t>> regions;

    void plant(uint64_t base, std::vector<uint8_t> bytes) {
        regions[base] = std::move(bytes);
    }

    size_t read(uint64_t addr, std::span<std::byte> out) const {
        for (const auto& [base, bytes] : regions) {
            if (addr < base || addr >= base + bytes.size())
                continue;
            size_t off = addr - base;
            size_t n = std::min(out.size(), bytes.size() - off);
            for (size_t i = 0; i < n; ++i)
                out[i] = static_cast<std::byte>(bytes[off + i]);
            return n;
        }
        return 0;
    }
};

const BasicBlock* block_at(const Function& fn, uint64_t addr) {
    for (const auto& bb : fn.blocks)
        if (bb.addr == addr)
            return &bb;
    return nullptr;
}

const Function* fn_at(const AnalysisResult& r, uint64_t addr) {
    for (const auto& fn : r.functions)
        if (fn.addr == addr)
            return &fn;
    return nullptr;
}

} // namespace

int main() {
    auto disasm = irida::disasm::make_x86_64_disassembler();
    assert(disasm != nullptr);

    Image img;
    // Function at 0x1000 with a conditional-branch diamond and a call:
    //   1000: push rbp            55
    //   1001: cmp  edi, 0         83 FF 00
    //   1004: jz   0x100c         74 06        (jump=0x100c, fail=0x1006)
    //   1006: call 0x2000         E8 F5 0F 00 00
    //   100b: nop                 90
    //   100c: pop  rbp            5D
    //   100d: ret                 C3
    img.plant(0x1000,
              {0x55, 0x83, 0xFF, 0x00, 0x74, 0x06, 0xE8, 0xF5, 0x0F, 0x00, 0x00, 0x90, 0x5D, 0xC3});
    // Callee at 0x2000: xor eax, eax; ret
    img.plant(0x2000, {0x31, 0xC0, 0xC3});

    Analyzer analyzer(
        *disasm, [&img](uint64_t addr, std::span<std::byte> out) { return img.read(addr, out); });

    std::vector<uint64_t> entries{0x1000};
    AnalysisResult result = analyzer.analyze(entries);

    // Two functions: the entry and the discovered callee.
    assert(result.functions.size() == 2);
    const Function* main_fn = fn_at(result, 0x1000);
    const Function* callee = fn_at(result, 0x2000);
    assert(main_fn != nullptr);
    assert(callee != nullptr);

    // The entry function must have exactly three blocks: the head (ends with the
    // conditional jump), the call block, and the return block.
    assert(main_fn->blocks.size() == 3);

    const BasicBlock* head = block_at(*main_fn, 0x1000);
    const BasicBlock* callblk = block_at(*main_fn, 0x1006);
    const BasicBlock* tail = block_at(*main_fn, 0x100c);
    assert(head != nullptr);
    assert(callblk != nullptr);
    assert(tail != nullptr);

    // Head: conditional jump to 0x100c, fall-through to 0x1006.
    assert(head->jump == 0x100c);
    assert(head->fail == 0x1006);
    assert(head->size == 6); // push(1) + cmp(3) + jz(2)

    // Call block: no branch, falls through into the tail leader.
    assert(callblk->jump == 0);
    assert(callblk->fail == 0x100c);

    // Tail: ret, no successors.
    assert(tail->jump == 0);
    assert(tail->fail == 0);

    // The call target was recorded both as a callee and as an xref.
    assert(main_fn->callees.size() == 1);
    assert(main_fn->callees[0] == 0x2000);

    bool found_call_xref = false;
    for (const auto& x : result.xrefs) {
        if (x.kind == XrefKind::Call && x.from == 0x1006 && x.to == 0x2000)
            found_call_xref = true;
    }
    assert(found_call_xref);

    // The callee is a single-block function ending in ret.
    assert(callee->blocks.size() == 1);
    assert(callee->blocks[0].addr == 0x2000);

    // Symbol naming: supply a name for the callee and re-analyze.
    analyzer.set_symbol_names([](uint64_t addr) -> std::optional<std::string> {
        if (addr == 0x2000)
            return std::string("my_helper");
        return std::nullopt;
    });
    AnalysisResult named = analyzer.analyze(entries);
    const Function* named_callee = fn_at(named, 0x2000);
    assert(named_callee != nullptr);
    assert(named_callee->name == "my_helper");

    const Function* unnamed = fn_at(named, 0x1000);
    assert(unnamed != nullptr);
    assert(unnamed->name == "fcn.1000");

    return 0;
}
