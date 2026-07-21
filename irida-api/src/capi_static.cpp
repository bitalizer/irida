// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// Static-file session: a binary opened for analysis with no live process
// behind it. Everything is served from the parsed file image — disassembly and
// analysis read bytes by mapping a virtual address to a file offset through the
// section table. Live-only operations (registers, threads, stepping) are inert.
#include "backend.hpp"
#include "irida/analysis/analysis.hpp"
#include "irida/binfmt/binary.hpp"
#include "irida/binfmt/strings.hpp"
#include "irida/disasm/disassembler.hpp"
#include "irida/disasm/instruction.hpp"
#include "irida/irida.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using irida::analysis::AnalysisResult;
using irida::analysis::Analyzer;
using irida::binfmt::BinaryInfo;
using irida::binfmt::BinaryParser;
using irida::binfmt::make_lief_parser;
using irida::binfmt::scan_strings;
using irida::disasm::Disassembler;
using irida::disasm::Instruction;
using irida::disasm::make_x86_64_disassembler;
using irida::disasm::OperandKind;

constexpr size_t kMaxInsnBytes = 15;

std::string format_bytes(std::span<const std::byte> bytes) {
    static const char* digits = "0123456789ABCDEF";
    std::string out;
    out.reserve(bytes.size() * 3);
    for (size_t i = 0; i < bytes.size(); ++i) {
        auto v = static_cast<unsigned>(bytes[i]);
        if (i != 0)
            out.push_back(' ');
        out.push_back(digits[(v >> 4) & 0xf]);
        out.push_back(digits[v & 0xf]);
    }
    return out;
}

std::optional<uint64_t> operand_target(const Instruction& insn) {
    for (const auto& op : insn.operands) {
        if ((op.kind == OperandKind::Immediate || op.kind == OperandKind::Pointer) && op.imm != 0)
            return op.imm;
    }
    for (const auto& op : insn.operands) {
        if (op.kind == OperandKind::Memory && op.mem.rip_relative)
            return static_cast<uint64_t>(insn.address + insn.length + op.mem.disp);
    }
    return std::nullopt;
}

} // namespace

// Owns the parsed binary, its raw bytes, and the scratch storage backing every
// borrowed pointer/array the vtable hands back (valid until the next call on
// the same ctx — same contract as the live session).
struct IridaStaticCtx {
    BinaryInfo bin;
    std::vector<std::byte> image; // raw file bytes
    std::unique_ptr<Disassembler> disasm = make_x86_64_disassembler();

    std::unordered_map<uint64_t, std::string> symbol_map;
    AnalysisResult analysis;

    std::vector<IridaInsnRow> insn_rows;
    std::vector<std::string> insn_text;
    std::vector<std::string> insn_bytes;
    std::vector<std::string> insn_annot;

    std::vector<IridaModule> module_rows;
    std::vector<std::string> module_names;

    std::vector<IridaSection> section_rows;
    std::vector<std::string> section_names;
    std::vector<IridaImport> import_rows;
    std::vector<std::string> import_names;
    std::vector<std::string> import_libs;
    std::vector<IridaExport> export_rows;
    std::vector<std::string> export_names;
    std::vector<IridaSymbol> symbol_rows;
    std::vector<std::string> symbol_names;
    std::vector<std::string> symbol_kinds;
    std::vector<IridaString> string_rows;
    std::vector<std::string> string_texts;

    std::vector<IridaFunction> function_rows;
    std::vector<std::string> function_names;
    std::vector<IridaBasicBlock> block_rows;
    std::vector<IridaXref> xref_rows;

    // Reads up to out.size() bytes at a virtual address by finding the section
    // that contains it and copying from the file image at the mapped offset.
    // Returns the number of bytes filled (0 if the address is not mapped).
    size_t read_at(uint64_t vaddr, std::span<std::byte> out) const {
        for (const auto& sec : bin.sections) {
            if (vaddr < sec.vaddr || vaddr >= sec.vaddr + sec.vsize)
                continue;
            uint64_t within = vaddr - sec.vaddr;
            if (within >= sec.size) // in virtual padding beyond the file data
                return 0;
            uint64_t file_off = sec.offset + within;
            if (file_off >= image.size())
                return 0;
            size_t avail = static_cast<size_t>(image.size() - file_off);
            size_t sec_avail = static_cast<size_t>(sec.size - within);
            size_t n = out.size();
            n = n < avail ? n : avail;
            n = n < sec_avail ? n : sec_avail;
            std::memcpy(out.data(), image.data() + file_off, n);
            return n;
        }
        return 0;
    }

    std::optional<std::string> resolve_symbol(uint64_t addr) const {
        auto it = symbol_map.find(addr);
        if (it == symbol_map.end())
            return std::nullopt;
        return it->second;
    }

    // Parses the file, loads its bytes, builds the symbol map, and runs
    // function/CFG analysis seeded from the entry point and every symbol.
    bool load(const std::string& path) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f)
            return false;
        std::streamsize len = f.tellg();
        if (len <= 0)
            return false;
        f.seekg(0);
        image.resize(static_cast<size_t>(len));
        if (!f.read(reinterpret_cast<char*>(image.data()), len))
            return false;

        auto parser = make_lief_parser();
        auto parsed = parser->parse_file(path);
        if (!parsed.has_value())
            return false;
        bin = std::move(parsed).value();

        for (const auto& sym : bin.symbols)
            symbol_map[sym.addr] = sym.name;
        for (const auto& exp : bin.exports)
            symbol_map[exp.addr] = "sym." + exp.name;
        for (const auto& imp : bin.imports)
            symbol_map[imp.iat_addr] = "sym.imp." + imp.name;

        // Analysis is deferred to irida_analyze() so session creation stays
        // cheap; the frontend runs it on a worker thread.
        return true;
    }

    void analyze() {
        std::vector<uint64_t> entries;
        if (bin.entry_point != 0)
            entries.push_back(bin.entry_point);
        for (const auto& [addr, name] : symbol_map)
            entries.push_back(addr);

        Analyzer analyzer(*disasm, [this](uint64_t addr, std::span<std::byte> buf) -> size_t {
            return read_at(addr, buf);
        });
        analyzer.set_symbol_names(
            [this](uint64_t addr) -> std::optional<std::string> { return resolve_symbol(addr); });
        analysis = analyzer.analyze(entries);
    }
};

namespace {

IridaStaticCtx* ctx_of(void* ctx) {
    return static_cast<IridaStaticCtx*>(ctx);
}

// --- live-only ops: inert for a static session ---
size_t static_registers(void*, const IridaRegister** out) {
    *out = nullptr;
    return 0;
}
size_t static_maps(void*, const IridaMemMap** out) {
    *out = nullptr;
    return 0;
}
size_t static_threads(void*, const IridaThread** out) {
    *out = nullptr;
    return 0;
}
IridaRunState static_run_state(void*) {
    return IRIDA_STOPPED;
}
uint64_t static_pc(void* ctx) {
    return ctx_of(ctx)->bin.entry_point;
}
uint64_t static_epoch(void*) {
    return 1;
}
void static_noop(void*) {}
size_t static_backtrace(void*, const IridaFrame** out) {
    *out = nullptr;
    return 0;
}
size_t static_breakpoints(void*, const IridaBreakpoint** out) {
    *out = nullptr;
    return 0;
}
void static_bp_toggle(void*, uint64_t) {}
void static_bp_set_enabled(void*, uint64_t, int) {}

size_t static_read_memory(void* ctx, uint64_t addr, uint8_t* buf, size_t len) {
    IridaStaticCtx* c = ctx_of(ctx);
    return c->read_at(addr, std::span(reinterpret_cast<std::byte*>(buf), len));
}

// One synthetic module: the file itself, based at its image base.
size_t static_modules(void* ctx, const IridaModule** out) {
    IridaStaticCtx* c = ctx_of(ctx);
    c->module_rows.clear();
    c->module_names.clear();
    c->module_names.push_back(c->bin.path);
    uint64_t span = 0;
    for (const auto& sec : c->bin.sections)
        span = (sec.vaddr + sec.vsize > span) ? sec.vaddr + sec.vsize : span;
    uint64_t size = span > c->bin.image_base ? span - c->bin.image_base : span;
    c->module_rows.push_back(IridaModule{c->module_names[0].c_str(), c->bin.image_base, size});
    *out = c->module_rows.data();
    return c->module_rows.size();
}

size_t static_disasm(void* ctx, uint64_t addr, size_t count, const IridaInsnRow** out) {
    IridaStaticCtx* c = ctx_of(ctx);
    c->insn_rows.clear();
    c->insn_text.clear();
    c->insn_bytes.clear();
    c->insn_annot.clear();
    *out = nullptr;
    if (count == 0)
        return 0;

    std::vector<std::byte> buf(count * kMaxInsnBytes);
    size_t got = c->read_at(addr, buf);
    if (got == 0)
        return 0;

    auto insns = c->disasm->decode_linear(std::span(buf.data(), got), addr, count);
    c->insn_text.reserve(insns.size());
    c->insn_bytes.reserve(insns.size());
    c->insn_annot.reserve(insns.size());
    c->insn_rows.reserve(insns.size());
    for (const auto& insn : insns) {
        c->insn_text.push_back(insn.text);
        size_t offset = static_cast<size_t>(insn.address - addr);
        size_t len = insn.length;
        len = offset + len <= got ? len : got - offset;
        c->insn_bytes.push_back(format_bytes(std::span(buf.data(), got).subspan(offset, len)));

        const char* annot = nullptr;
        if (auto tgt = operand_target(insn)) {
            if (auto name = c->resolve_symbol(tgt.value())) {
                c->insn_annot.push_back(std::move(name).value());
                annot = c->insn_annot.back().c_str();
            }
        }
        c->insn_rows.push_back(IridaInsnRow{insn.address, c->insn_text.back().c_str(), annot,
                                            c->insn_bytes.back().c_str()});
    }
    *out = c->insn_rows.empty() ? nullptr : c->insn_rows.data();
    return c->insn_rows.size();
}

size_t static_sections(void* ctx, const IridaSection** out) {
    IridaStaticCtx* c = ctx_of(ctx);
    c->section_rows.clear();
    c->section_names.clear();
    const auto& secs = c->bin.sections;
    c->section_names.reserve(secs.size());
    for (const auto& sec : secs)
        c->section_names.push_back(sec.name);
    c->section_rows.reserve(secs.size());
    for (size_t i = 0; i < secs.size(); ++i)
        c->section_rows.push_back(
            IridaSection{c->section_names[i].c_str(), secs[i].vaddr, secs[i].vsize, secs[i].perms});
    *out = c->section_rows.empty() ? nullptr : c->section_rows.data();
    return c->section_rows.size();
}

size_t static_imports(void* ctx, const IridaImport** out) {
    IridaStaticCtx* c = ctx_of(ctx);
    c->import_rows.clear();
    c->import_names.clear();
    c->import_libs.clear();
    const auto& imports = c->bin.imports;
    c->import_names.reserve(imports.size());
    c->import_libs.reserve(imports.size());
    for (const auto& imp : imports) {
        c->import_names.push_back(imp.name);
        c->import_libs.push_back(imp.library);
    }
    c->import_rows.reserve(imports.size());
    for (size_t i = 0; i < imports.size(); ++i)
        c->import_rows.push_back(IridaImport{c->import_names[i].c_str(), c->import_libs[i].c_str(),
                                             imports[i].iat_addr});
    *out = c->import_rows.empty() ? nullptr : c->import_rows.data();
    return c->import_rows.size();
}

size_t static_exports(void* ctx, const IridaExport** out) {
    IridaStaticCtx* c = ctx_of(ctx);
    c->export_rows.clear();
    c->export_names.clear();
    const auto& exports = c->bin.exports;
    c->export_names.reserve(exports.size());
    for (const auto& exp : exports)
        c->export_names.push_back(exp.name);
    c->export_rows.reserve(exports.size());
    for (size_t i = 0; i < exports.size(); ++i)
        c->export_rows.push_back(
            IridaExport{c->export_names[i].c_str(), exports[i].addr, exports[i].ordinal});
    *out = c->export_rows.empty() ? nullptr : c->export_rows.data();
    return c->export_rows.size();
}

size_t static_symbols(void* ctx, const IridaSymbol** out) {
    IridaStaticCtx* c = ctx_of(ctx);
    c->symbol_rows.clear();
    c->symbol_names.clear();
    c->symbol_kinds.clear();
    const auto& symbols = c->bin.symbols;
    c->symbol_names.reserve(symbols.size());
    c->symbol_kinds.reserve(symbols.size());
    for (const auto& sym : symbols) {
        c->symbol_names.push_back(sym.name);
        c->symbol_kinds.push_back(sym.kind);
    }
    c->symbol_rows.reserve(symbols.size());
    for (size_t i = 0; i < symbols.size(); ++i)
        c->symbol_rows.push_back(
            IridaSymbol{c->symbol_names[i].c_str(), symbols[i].addr, c->symbol_kinds[i].c_str()});
    *out = c->symbol_rows.empty() ? nullptr : c->symbol_rows.data();
    return c->symbol_rows.size();
}

size_t static_strings(void* ctx, const IridaString** out) {
    IridaStaticCtx* c = ctx_of(ctx);
    c->string_rows.clear();
    c->string_texts.clear();
    auto found = scan_strings(c->image, c->bin.image_base);
    c->string_texts.reserve(found.size());
    for (const auto& s : found)
        c->string_texts.push_back(s.text);
    c->string_rows.reserve(found.size());
    for (size_t i = 0; i < found.size(); ++i)
        c->string_rows.push_back(IridaString{found[i].addr, c->string_texts[i].c_str()});
    *out = c->string_rows.empty() ? nullptr : c->string_rows.data();
    return c->string_rows.size();
}

void static_analyze(void* ctx) {
    ctx_of(ctx)->analyze();
}

size_t static_functions(void* ctx, const IridaFunction** out) {
    IridaStaticCtx* c = ctx_of(ctx);
    c->function_rows.clear();
    c->function_names.clear();
    const auto& fns = c->analysis.functions;
    c->function_names.reserve(fns.size());
    for (const auto& fn : fns)
        c->function_names.push_back(fn.name);
    c->function_rows.reserve(fns.size());
    for (size_t i = 0; i < fns.size(); ++i)
        c->function_rows.push_back(IridaFunction{
            fns[i].addr, fns[i].size, c->function_names[i].c_str(), fns[i].blocks.size()});
    *out = c->function_rows.empty() ? nullptr : c->function_rows.data();
    return c->function_rows.size();
}

size_t static_function_blocks(void* ctx, uint64_t fn_addr, const IridaBasicBlock** out) {
    IridaStaticCtx* c = ctx_of(ctx);
    c->block_rows.clear();
    for (const auto& fn : c->analysis.functions) {
        if (fn.addr != fn_addr)
            continue;
        c->block_rows.reserve(fn.blocks.size());
        for (const auto& bb : fn.blocks)
            c->block_rows.push_back(IridaBasicBlock{bb.addr, bb.size, bb.jump, bb.fail});
        break;
    }
    *out = c->block_rows.empty() ? nullptr : c->block_rows.data();
    return c->block_rows.size();
}

size_t static_xrefs_to(void* ctx, uint64_t addr, const IridaXref** out) {
    IridaStaticCtx* c = ctx_of(ctx);
    c->xref_rows.clear();
    for (const auto& x : c->analysis.xrefs) {
        if (x.to != addr)
            continue;
        c->xref_rows.push_back(IridaXref{x.from, x.to, static_cast<int>(x.kind)});
    }
    *out = c->xref_rows.empty() ? nullptr : c->xref_rows.data();
    return c->xref_rows.size();
}

const IridaBackendVTable kStaticVTable = {static_registers,
                                          static_modules,
                                          static_maps,
                                          static_threads,
                                          static_disasm,
                                          static_run_state,
                                          static_pc,
                                          static_epoch,
                                          static_noop, // step_into
                                          static_noop, // step_over
                                          static_noop, // step_out
                                          static_noop, // cont
                                          static_noop, // brk
                                          static_noop, // restart
                                          static_noop, // stop
                                          static_read_memory,
                                          static_breakpoints,
                                          static_bp_toggle,
                                          static_bp_set_enabled,
                                          static_backtrace,
                                          static_sections,
                                          static_imports,
                                          static_exports,
                                          static_symbols,
                                          static_strings,
                                          static_analyze,
                                          static_functions,
                                          static_function_blocks,
                                          static_xrefs_to};

} // namespace

extern "C" IridaSession* irida_session_create_file(const char* path) {
    if (!path)
        return nullptr;
    auto* ctx = new (std::nothrow) IridaStaticCtx();
    if (!ctx)
        return nullptr;
    if (!ctx->load(path)) {
        delete ctx;
        return nullptr;
    }
    IridaSession* session = irida_session_create(&kStaticVTable, ctx);
    if (!session) {
        delete ctx;
        return nullptr;
    }
    return session;
}

extern "C" void irida_session_destroy_static(IridaSession* s) {
    if (!s)
        return;
    delete static_cast<IridaStaticCtx*>(s->ctx);
    irida_session_destroy(s);
}
