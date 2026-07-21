// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include "irida/disasm/disassembler.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace irida::analysis {

// A maximal straight-line run of instructions with a single entry and a single
// exit. The exit is described by up to two outgoing edges:
//   jump - taken-branch successor (branch target of a Jump/CondJump), or 0
//   fail - fall-through successor (the next instruction after a CondJump), or 0
// An unconditional Jump sets only jump; a CondJump sets both; a Return sets
// neither; a block that ends only because a later block starts mid-run
// (a split) sets only fail.
struct BasicBlock {
    uint64_t addr = 0;
    uint64_t size = 0; // total bytes spanned by the block's instructions
    uint64_t jump = 0;
    uint64_t fail = 0;
    std::vector<uint64_t> instr_addrs; // start address of each instruction, in order
};

// A discovered subroutine: an entry address, its reachable basic blocks, and
// the set of addresses it calls directly.
struct Function {
    uint64_t addr = 0;
    uint64_t size = 0; // sum of block sizes
    std::string name;  // "fcn.<hex>" unless a symbol supplies a real name
    std::vector<BasicBlock> blocks;
    std::vector<uint64_t> callees; // direct call targets, deduplicated
};

enum class XrefKind { Call, Jump, Data };

// A reference from one address to another (site -> target).
struct Xref {
    uint64_t from = 0;
    uint64_t to = 0;
    XrefKind kind = XrefKind::Call;
};

// Reads up to `out.size()` bytes of the program image starting at `addr`.
// Returns the number of bytes actually filled (0 if the address is unmapped).
// This is the analyzer's sole window onto the program, so it works identically
// against a live target's memory and a static binary image.
using ByteReader = std::function<size_t(uint64_t addr, std::span<std::byte> out)>;

struct AnalysisResult {
    std::vector<Function> functions;
    std::vector<Xref> xrefs;
};

// Recursive-descent analyzer. Given entry points (program entry, exported
// symbols, discovered call targets), it disassembles reachable code, splits it
// into basic blocks on control-flow boundaries, and records cross-references.
class Analyzer {
  public:
    Analyzer(disasm::Disassembler& disasm, ByteReader reader);

    // Analyze starting from the given entry points. Every direct call target
    // discovered along the way is treated as a further function entry, so a
    // single entry point can yield the whole reachable call graph.
    AnalysisResult analyze(std::span<const uint64_t> entry_points);

    // Optional: supply known symbol names so discovered functions can be named
    // (addr -> name). Functions with no matching symbol get "fcn.<hex>".
    void set_symbol_names(std::function<std::optional<std::string>(uint64_t)> lookup);

  private:
    disasm::Disassembler& disasm_;
    ByteReader reader_;
    std::function<std::optional<std::string>(uint64_t)> symbol_lookup_;
};

} // namespace irida::analysis
