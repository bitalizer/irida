// SPDX-License-Identifier: BUSL-1.1
#include "backend.hpp"
#include "irida/backend/native_backend.hpp"
#include "irida/backend/types.hpp"
#include "irida/binfmt/binary.hpp"
#include "irida/binfmt/strings.hpp"
#include "irida/disasm/disassembler.hpp"
#include "irida/disasm/instruction.hpp"
#include "irida/irida.h"
#include "irida/target/target.hpp"
#include <cstring>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using irida::backend::AttachMethod;
using irida::backend::AttachSpec;
using irida::backend::make_native_backend;
using irida::binfmt::BinaryInfo;
using irida::binfmt::BinaryParser;
using irida::binfmt::make_lief_parser;
using irida::binfmt::scan_strings;
using irida::disasm::Disassembler;
using irida::disasm::Instruction;
using irida::disasm::make_x86_64_disassembler;
using irida::disasm::OperandKind;
using irida::target::Target;

constexpr size_t kMaxInsnBytes = 15; // longest possible x86-64 instruction
constexpr size_t kMaxBacktraceFrames = 64;
constexpr uint64_t kStringsScanCap = 1024 * 1024; // cap the memory read for the strings scan

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

// Builds an addr->display-name map from a parsed BinaryInfo, in the order
// symbols, exports, imports (each insertion overwrites the previous), so
// that on an address collision imports (sym.imp.X) win over exports, which
// win over symbols.
//
// module_base/file_base rebase file addresses (RVA/file-VA, as produced by
// parsing the on-disk PE) into runtime addresses (module_base + (file_addr -
// file_base)), so the map can be looked up directly with live target
// addresses. Pass module_base == file_base to keep addresses unrebased (e.g.
// when no live module base is known, or the addresses are already runtime,
// as in the mock).
std::unordered_map<uint64_t, std::string>
build_symbol_map(const BinaryInfo& bin, uint64_t module_base, uint64_t file_base) {
    std::unordered_map<uint64_t, std::string> map;
    auto rebase = [&](uint64_t file_addr) { return module_base + (file_addr - file_base); };

    for (const auto& sym : bin.symbols)
        map[rebase(sym.addr)] = sym.name;

    for (const auto& exp : bin.exports)
        map[rebase(exp.addr)] = "sym." + exp.name;

    for (const auto& imp : bin.imports)
        map[rebase(imp.iat_addr)] = "sym.imp." + imp.name;

    return map;
}

// Returns the address an instruction refers to (call/jmp/lea/data-ref target)
// if it plainly has one, else nullopt. Immediate/Pointer operands already
// carry the resolved absolute target (Zydis resolves rel8/rel32 displacements
// against the instruction address at decode time). A rip-relative memory
// operand's target is address-of-next-instruction + disp.
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

// Owns a real attached Target plus the disassembler and the scratch storage
// backing every borrowed pointer/array the vtable hands back to the ABI
// caller (valid until the next call on the same ctx, same contract as
// mock_backend.cpp).
struct IridaCoreCtx {
    Target target;
    std::unique_ptr<Disassembler> disasm;

    std::vector<IridaRegister> reg_rows;
    std::vector<std::string> reg_names;

    std::vector<IridaInsnRow> insn_rows;
    std::vector<std::string> insn_text;
    std::vector<std::string> insn_bytes;
    std::vector<std::string> insn_annot;

    std::vector<IridaBreakpoint> bp_rows;

    std::vector<IridaFrame> frame_rows;

    std::vector<IridaModule> module_rows;
    std::vector<std::string> module_names;

    std::vector<IridaMemMap> map_rows;
    std::vector<std::string> map_names;

    std::vector<IridaThread> thread_rows;

    // Static-binary model: the main module's file is parsed once (lazily,
    // on first sections/imports/exports/symbols/strings call) and cached.
    std::unique_ptr<BinaryParser> binary_parser = make_lief_parser();
    BinaryInfo main_bin;
    bool binary_parsed = false;

    // addr(runtime) -> display name, built once alongside main_bin.
    std::unordered_map<uint64_t, std::string> symbol_map;

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

    explicit IridaCoreCtx(Target t) : target(std::move(t)), disasm(make_x86_64_disassembler()) {}

    // Lazily parses the main module (first module reported by the target)
    // as a static binary. Returns false (no throw) if there is no module or
    // the parse fails, e.g. under a synthetic/mock target with no real file
    // on disk.
    bool ensure_binary_parsed() {
        if (binary_parsed)
            return main_bin.sections.size() + main_bin.imports.size() + main_bin.exports.size() +
                       main_bin.symbols.size() >
                   0;

        binary_parsed = true;

        auto modules = target.modules();
        if (!modules.has_value() || modules.value().empty())
            return false;

        auto parsed = binary_parser->parse_file(modules.value()[0].name);
        if (!parsed.has_value())
            return false;

        main_bin = std::move(parsed).value();

        // main_bin's addresses are file-relative (RVA/file-VA using
        // main_bin.image_base); the live target sees runtime addresses
        // (the module's load base + RVA). Rebase onto the first module's
        // live base so the map can be looked up with runtime addresses
        // straight from decoded instructions.
        uint64_t module_base = modules.value()[0].base;
        symbol_map = build_symbol_map(main_bin, module_base, main_bin.image_base);
        return true;
    }

    // Resolves a runtime address to a display name (sym.imp.X / sym.X /
    // plain symbol name) using the cached symbol_map, or nullopt if unknown.
    std::optional<std::string> resolve_symbol(uint64_t addr) {
        if (!ensure_binary_parsed())
            return std::nullopt;
        auto it = symbol_map.find(addr);
        if (it == symbol_map.end())
            return std::nullopt;
        return it->second;
    }
};

namespace {

IridaCoreCtx* ctx_of(void* ctx) {
    return static_cast<IridaCoreCtx*>(ctx);
}

size_t core_registers(void* ctx, const IridaRegister** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    const auto& all = c->target.registers().all();

    c->reg_names.clear();
    c->reg_names.reserve(all.size());
    for (const auto& [name, value] : all)
        c->reg_names.push_back(name);

    c->reg_rows.clear();
    c->reg_rows.reserve(all.size());
    for (size_t i = 0; i < all.size(); ++i)
        c->reg_rows.push_back(IridaRegister{c->reg_names[i].c_str(), all[i].second, nullptr});

    *out = c->reg_rows.empty() ? nullptr : c->reg_rows.data();
    return c->reg_rows.size();
}

size_t core_modules(void* ctx, const IridaModule** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    *out = nullptr;

    auto modules = c->target.modules();
    if (!modules.has_value())
        return 0;

    c->module_names.clear();
    c->module_names.reserve(modules.value().size());
    for (const auto& m : modules.value())
        c->module_names.push_back(m.name);

    c->module_rows.clear();
    c->module_rows.reserve(modules.value().size());
    for (size_t i = 0; i < modules.value().size(); ++i) {
        const auto& m = modules.value()[i];
        c->module_rows.push_back(IridaModule{c->module_names[i].c_str(), m.base, m.size});
    }

    *out = c->module_rows.empty() ? nullptr : c->module_rows.data();
    return c->module_rows.size();
}

size_t core_maps(void* ctx, const IridaMemMap** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    *out = nullptr;

    auto maps = c->target.maps();
    if (!maps.has_value())
        return 0;

    c->map_names.clear();
    c->map_names.reserve(maps.value().size());
    for (const auto& m : maps.value())
        c->map_names.push_back(m.name);

    c->map_rows.clear();
    c->map_rows.reserve(maps.value().size());
    for (size_t i = 0; i < maps.value().size(); ++i) {
        const auto& m = maps.value()[i];
        c->map_rows.push_back(IridaMemMap{m.start, m.end, m.perms, c->map_names[i].c_str()});
    }

    *out = c->map_rows.empty() ? nullptr : c->map_rows.data();
    return c->map_rows.size();
}

size_t core_threads(void* ctx, const IridaThread** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    *out = nullptr;

    auto threads = c->target.threads();
    if (!threads.has_value())
        return 0;

    c->thread_rows.clear();
    c->thread_rows.reserve(threads.value().size());
    for (const auto& t : threads.value())
        c->thread_rows.push_back(IridaThread{t.tid, t.pc, t.current ? 1 : 0});

    *out = c->thread_rows.empty() ? nullptr : c->thread_rows.data();
    return c->thread_rows.size();
}

size_t core_disasm(void* ctx, uint64_t addr, size_t count, const IridaInsnRow** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    c->insn_rows.clear();
    c->insn_text.clear();
    c->insn_bytes.clear();
    c->insn_annot.clear();
    *out = nullptr;

    if (count == 0)
        return 0;

    auto bytes = c->target.read_memory(addr, count * kMaxInsnBytes);
    if (!bytes.has_value())
        return 0;

    auto insns = c->disasm->decode_linear(bytes.value(), addr, count);
    c->insn_text.reserve(insns.size());
    c->insn_bytes.reserve(insns.size());
    c->insn_annot.reserve(insns.size());
    c->insn_rows.reserve(insns.size());
    for (const auto& insn : insns) {
        c->insn_text.push_back(insn.text);

        size_t offset = static_cast<size_t>(insn.address - addr);
        size_t len = insn.length;
        len = offset + len <= bytes.value().size() ? len : bytes.value().size() - offset;
        c->insn_bytes.push_back(format_bytes(std::span(bytes.value()).subspan(offset, len)));

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

size_t core_backtrace(void* ctx, const IridaFrame** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    c->frame_rows.clear();
    *out = nullptr;

    const auto& regs = c->target.registers();
    uint64_t pc = regs.get("rip").value_or(0);
    uint64_t fp = regs.get("rbp").value_or(0);

    c->frame_rows.push_back(IridaFrame{pc, fp});

    for (size_t i = 0; i < kMaxBacktraceFrames; ++i) {
        auto frame_mem = c->target.read_memory(fp, 16);
        if (!frame_mem.has_value() || frame_mem.value().size() < 16)
            break;

        const auto* bytes = frame_mem.value().data();
        uint64_t saved_rbp = 0;
        uint64_t ret_addr = 0;
        std::memcpy(&saved_rbp, bytes, sizeof(saved_rbp));
        std::memcpy(&ret_addr, bytes + 8, sizeof(ret_addr));

        if (saved_rbp <= fp || ret_addr == 0)
            break;

        c->frame_rows.push_back(IridaFrame{ret_addr, saved_rbp});
        fp = saved_rbp;
    }

    *out = c->frame_rows.empty() ? nullptr : c->frame_rows.data();
    return c->frame_rows.size();
}

IridaRunState core_run_state(void* /*ctx*/) {
    // Target is always at a stop between ABI calls (cont()/step() only
    // return once the backend has re-halted).
    return IRIDA_STOPPED;
}

uint64_t core_pc(void* ctx) {
    IridaCoreCtx* c = ctx_of(ctx);
    return c->target.registers().get("rip").value_or(0);
}

uint64_t core_state_epoch(void* ctx) {
    return ctx_of(ctx)->target.epoch();
}

void core_step_into(void* ctx) {
    (void)ctx_of(ctx)->target.step();
}

void core_step_over(void* ctx) {
    // call-detection deferred: alias to a plain single step for now.
    (void)ctx_of(ctx)->target.step();
}

void core_step_out(void* ctx) {
    // call-detection deferred: alias to a plain single step for now.
    (void)ctx_of(ctx)->target.step();
}

void core_cont(void* ctx) {
    (void)ctx_of(ctx)->target.cont();
}

void core_brk(void* /*ctx*/) {
    // NativeBackend has no async stop wired through Target yet; no-op.
}

void core_restart(void* /*ctx*/) {
    // Not supported yet for a native-attached session; no-op.
}

void core_stop(void* /*ctx*/) {
    // Detach is owned by session teardown (irida_session_destroy_native); no-op here.
}

size_t core_read_memory(void* ctx, uint64_t addr, uint8_t* buf, size_t len) {
    IridaCoreCtx* c = ctx_of(ctx);
    auto bytes = c->target.read_memory(addr, len);
    if (!bytes.has_value())
        return 0;
    size_t got = bytes.value().size() < len ? bytes.value().size() : len;
    std::memcpy(buf, bytes.value().data(), got);
    return got;
}

size_t core_breakpoints(void* ctx, const IridaBreakpoint** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    *out = c->bp_rows.empty() ? nullptr : c->bp_rows.data();
    return c->bp_rows.size();
}

void core_bp_toggle(void* /*ctx*/, uint64_t /*addr*/) {
    // Backend set_breakpoint/remove_breakpoint wiring deferred; tracked list stays empty.
}

void core_bp_set_enabled(void* /*ctx*/, uint64_t /*addr*/, int /*enabled*/) {
    // Backend set_breakpoint/remove_breakpoint wiring deferred; tracked list stays empty.
}

size_t core_sections(void* ctx, const IridaSection** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    c->section_rows.clear();
    c->section_names.clear();
    *out = nullptr;

    if (!c->ensure_binary_parsed())
        return 0;

    const auto& sections = c->main_bin.sections;
    c->section_names.reserve(sections.size());
    for (const auto& sec : sections)
        c->section_names.push_back(sec.name);

    c->section_rows.reserve(sections.size());
    for (size_t i = 0; i < sections.size(); ++i)
        c->section_rows.push_back(IridaSection{c->section_names[i].c_str(), sections[i].vaddr,
                                               sections[i].vsize, sections[i].perms});

    *out = c->section_rows.empty() ? nullptr : c->section_rows.data();
    return c->section_rows.size();
}

size_t core_imports(void* ctx, const IridaImport** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    c->import_rows.clear();
    c->import_names.clear();
    c->import_libs.clear();
    *out = nullptr;

    if (!c->ensure_binary_parsed())
        return 0;

    const auto& imports = c->main_bin.imports;
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

size_t core_exports(void* ctx, const IridaExport** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    c->export_rows.clear();
    c->export_names.clear();
    *out = nullptr;

    if (!c->ensure_binary_parsed())
        return 0;

    const auto& exports = c->main_bin.exports;
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

size_t core_symbols(void* ctx, const IridaSymbol** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    c->symbol_rows.clear();
    c->symbol_names.clear();
    c->symbol_kinds.clear();
    *out = nullptr;

    if (!c->ensure_binary_parsed())
        return 0;

    const auto& symbols = c->main_bin.symbols;
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

size_t core_strings(void* ctx, const IridaString** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    c->string_rows.clear();
    c->string_texts.clear();
    *out = nullptr;

    auto modules = c->target.modules();
    if (!modules.has_value() || modules.value().empty())
        return 0;

    const auto& mod = modules.value()[0];
    uint64_t len = mod.size < kStringsScanCap ? mod.size : kStringsScanCap;
    if (len == 0)
        return 0;

    auto bytes = c->target.read_memory(mod.base, len);
    if (!bytes.has_value())
        return 0;

    auto found = scan_strings(bytes.value(), mod.base);
    c->string_texts.reserve(found.size());
    for (const auto& f : found)
        c->string_texts.push_back(f.text);

    c->string_rows.reserve(found.size());
    for (size_t i = 0; i < found.size(); ++i)
        c->string_rows.push_back(IridaString{found[i].addr, c->string_texts[i].c_str()});

    *out = c->string_rows.empty() ? nullptr : c->string_rows.data();
    return c->string_rows.size();
}

const IridaBackendVTable kCoreVTable = {
    core_registers,   core_modules,     core_maps,        core_threads,        core_disasm,
    core_run_state,   core_pc,          core_state_epoch, core_step_into,      core_step_over,
    core_step_out,    core_cont,        core_brk,         core_restart,        core_stop,
    core_read_memory, core_breakpoints, core_bp_toggle,   core_bp_set_enabled, core_backtrace,
    core_sections,    core_imports,     core_exports,     core_symbols,        core_strings};

} // namespace

extern "C" IridaSession* irida_session_create_native(uint32_t pid) {
    AttachSpec spec;
    spec.method = AttachMethod::NativeProcess;
    spec.pid = pid;

    auto backend = make_native_backend();
    auto connected = backend->attach(spec);
    if (!connected.has_value())
        return nullptr;

    auto attached = Target::attach(std::move(backend));
    if (!attached.has_value())
        return nullptr;

    auto* ctx = new (std::nothrow) IridaCoreCtx(std::move(attached).value());
    if (!ctx)
        return nullptr;

    IridaSession* session = irida_session_create(&kCoreVTable, ctx);
    if (!session) {
        delete ctx;
        return nullptr;
    }
    return session;
}

extern "C" void irida_session_destroy_native(IridaSession* s) {
    if (!s)
        return;
    // s->ctx is the IridaCoreCtx* created above; irida_session_destroy only
    // frees the session wrapper, so free the ctx first while it's still
    // reachable through the (soon-to-be-freed) session.
    delete static_cast<IridaCoreCtx*>(s->ctx);
    irida_session_destroy(s);
}
