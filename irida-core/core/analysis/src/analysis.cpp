// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#include "irida/analysis/analysis.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <deque>
#include <map>
#include <set>
#include <unordered_set>

namespace irida::analysis {

using disasm::Flow;
using disasm::Instruction;

namespace {

// Largest single x86-64 instruction is 15 bytes; a comfortable window keeps
// decode_one fed without a memory round-trip per instruction.
constexpr size_t kReadWindow = 256;

std::string default_name(uint64_t addr) {
    std::array<char, 32> buf{};
    std::snprintf(buf.data(), buf.size(), "fcn.%llx", static_cast<unsigned long long>(addr));
    return std::string(buf.data());
}

// Discovers one function by recursive descent from `entry`. Fills `fn` with its
// basic blocks and appends any direct call targets it finds to `out_callees`
// and control-flow references to `out_xrefs`.
class FunctionBuilder {
  public:
    FunctionBuilder(disasm::Disassembler& disasm, const ByteReader& reader)
        : disasm_(disasm), reader_(reader) {}

    void build(uint64_t entry, Function& fn, std::vector<uint64_t>& out_callees,
               std::vector<Xref>& out_xrefs) {
        leaders_.insert(entry);
        std::deque<uint64_t> work{entry};
        std::unordered_set<uint64_t> queued{entry};

        while (!work.empty()) {
            uint64_t start = work.front();
            work.pop_front();
            if (blocks_.contains(start))
                continue;

            BasicBlock bb = trace_block(start, out_callees, out_xrefs);
            blocks_.emplace(bb.addr, bb);

            auto enqueue = [&](uint64_t succ) {
                if (succ != 0 && !queued.contains(succ)) {
                    queued.insert(succ);
                    work.push_back(succ);
                }
            };
            enqueue(bb.jump);
            enqueue(bb.fail);
        }

        finalize(entry, fn);
    }

  private:
    // Decodes a straight-line run from `start`, stopping at the first
    // control-flow instruction or at an address that is already the leader of
    // another block (a fall-through into a known block). Registers new leaders
    // for branch targets so they become blocks in a later iteration.
    BasicBlock trace_block(uint64_t start, std::vector<uint64_t>& out_callees,
                           std::vector<Xref>& out_xrefs) {
        BasicBlock bb;
        bb.addr = start;
        uint64_t pc = start;
        std::array<std::byte, kReadWindow> window{};

        while (true) {
            // Stop before an address that already leads another block: the run
            // falls straight through into it.
            if (pc != start && leaders_.contains(pc)) {
                bb.fail = pc;
                break;
            }

            size_t got = reader_(pc, window);
            if (got == 0)
                break;

            auto decoded = disasm_.decode_one(std::span(window.data(), got), pc);
            if (!decoded.has_value())
                break;

            Instruction insn = std::move(decoded).value();
            bb.instr_addrs.push_back(insn.address);
            bb.size += insn.length;
            uint64_t next = insn.address + insn.length;

            switch (insn.flow) {
            case Flow::Jump:
                if (insn.branch_target) {
                    out_xrefs.push_back({insn.address, insn.branch_target, XrefKind::Jump});
                    add_leader(insn.branch_target);
                    bb.jump = insn.branch_target;
                }
                return bb;
            case Flow::CondJump:
                if (insn.branch_target) {
                    out_xrefs.push_back({insn.address, insn.branch_target, XrefKind::Jump});
                    add_leader(insn.branch_target);
                    bb.jump = insn.branch_target;
                }
                add_leader(next);
                bb.fail = next;
                return bb;
            case Flow::Call:
                if (insn.branch_target) {
                    out_xrefs.push_back({insn.address, insn.branch_target, XrefKind::Call});
                    out_callees.push_back(insn.branch_target);
                }
                // A call returns: keep decoding the same block.
                break;
            case Flow::Return:
                return bb;
            case Flow::Sequential:
                break;
            }

            pc = next;
        }
        return bb;
    }

    void add_leader(uint64_t addr) {
        leaders_.insert(addr);
    }

    // A branch target may land in the middle of an already-traced block. Split
    // every such block so leaders always begin a block, then assemble the
    // function's block list sorted by address.
    void finalize(uint64_t entry, Function& fn) {
        bool changed = true;
        while (changed) {
            changed = false;
            for (auto& [addr, bb] : blocks_) {
                for (uint64_t leader : leaders_) {
                    if (leader <= bb.addr || leader >= bb.addr + bb.size)
                        continue;
                    if (!is_instr_boundary(bb, leader))
                        continue;
                    if (split_block(bb, leader))
                        changed = true;
                    break;
                }
                if (changed)
                    break;
            }
        }

        fn.addr = entry;
        fn.name = default_name(entry);
        for (auto& [addr, bb] : blocks_) {
            fn.size += bb.size;
            fn.blocks.push_back(bb);
        }
        std::sort(fn.blocks.begin(), fn.blocks.end(),
                  [](const BasicBlock& a, const BasicBlock& b) { return a.addr < b.addr; });
    }

    static bool is_instr_boundary(const BasicBlock& bb, uint64_t addr) {
        return std::find(bb.instr_addrs.begin(), bb.instr_addrs.end(), addr) !=
               bb.instr_addrs.end();
    }

    // Cuts `bb` at `leader`: the head keeps [bb.addr, leader) and falls through
    // to the new tail [leader, ...) which inherits bb's original successors.
    bool split_block(BasicBlock& bb, uint64_t leader) {
        BasicBlock tail;
        tail.addr = leader;
        tail.jump = bb.jump;
        tail.fail = bb.fail;

        uint64_t head_size = 0;
        std::vector<uint64_t> head_instrs;
        std::vector<uint64_t> tail_instrs;
        for (uint64_t ia : bb.instr_addrs) {
            if (ia < leader) {
                head_instrs.push_back(ia);
            } else {
                tail_instrs.push_back(ia);
            }
        }
        head_size = leader - bb.addr;
        tail.size = bb.size - head_size;
        tail.instr_addrs = std::move(tail_instrs);

        bb.size = head_size;
        bb.instr_addrs = std::move(head_instrs);
        bb.jump = 0;
        bb.fail = leader;

        blocks_.emplace(tail.addr, std::move(tail));
        return true;
    }

    disasm::Disassembler& disasm_;
    const ByteReader& reader_;
    std::set<uint64_t> leaders_;
    std::map<uint64_t, BasicBlock> blocks_;
};

} // namespace

Analyzer::Analyzer(disasm::Disassembler& disasm, ByteReader reader)
    : disasm_(disasm), reader_(std::move(reader)) {}

void Analyzer::set_symbol_names(std::function<std::optional<std::string>(uint64_t)> lookup) {
    symbol_lookup_ = std::move(lookup);
}

AnalysisResult Analyzer::analyze(std::span<const uint64_t> entry_points) {
    AnalysisResult result;
    std::unordered_set<uint64_t> analyzed;
    std::deque<uint64_t> work(entry_points.begin(), entry_points.end());
    std::unordered_set<uint64_t> queued(entry_points.begin(), entry_points.end());

    while (!work.empty()) {
        uint64_t entry = work.front();
        work.pop_front();
        if (entry == 0 || analyzed.contains(entry))
            continue;
        analyzed.insert(entry);

        Function fn;
        std::vector<uint64_t> callees;
        FunctionBuilder builder(disasm_, reader_);
        builder.build(entry, fn, callees, result.xrefs);

        if (fn.blocks.empty())
            continue;

        std::sort(callees.begin(), callees.end());
        callees.erase(std::unique(callees.begin(), callees.end()), callees.end());
        fn.callees = callees;

        if (symbol_lookup_) {
            if (auto name = symbol_lookup_(entry))
                fn.name = *name;
        }
        result.functions.push_back(std::move(fn));

        for (uint64_t callee : callees) {
            if (!queued.contains(callee)) {
                queued.insert(callee);
                work.push_back(callee);
            }
        }
    }

    std::sort(result.functions.begin(), result.functions.end(),
              [](const Function& a, const Function& b) { return a.addr < b.addr; });
    return result;
}

} // namespace irida::analysis
