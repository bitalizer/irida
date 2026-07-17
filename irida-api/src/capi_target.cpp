// SPDX-License-Identifier: BUSL-1.1
#include "backend.hpp"
#include "irida/backend/native_backend.hpp"
#include "irida/backend/types.hpp"
#include "irida/disasm/disassembler.hpp"
#include "irida/irida.h"
#include "irida/target/target.hpp"
#include <cstring>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {

using irida::backend::AttachMethod;
using irida::backend::AttachSpec;
using irida::backend::make_native_backend;
using irida::disasm::Disassembler;
using irida::disasm::make_x86_64_disassembler;
using irida::target::Target;

constexpr size_t kMaxInsnBytes = 15; // longest possible x86-64 instruction

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

    std::vector<IridaBreakpoint> bp_rows;

    explicit IridaCoreCtx(Target t) : target(std::move(t)), disasm(make_x86_64_disassembler()) {}
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

size_t core_modules(void* /*ctx*/, const IridaModule** out) {
    *out = nullptr;
    return 0;
}

size_t core_disasm(void* ctx, uint64_t addr, size_t count, const IridaInsnRow** out) {
    IridaCoreCtx* c = ctx_of(ctx);
    c->insn_rows.clear();
    c->insn_text.clear();
    *out = nullptr;

    if (count == 0)
        return 0;

    auto bytes = c->target.read_memory(addr, count * kMaxInsnBytes);
    if (!bytes.has_value())
        return 0;

    auto insns = c->disasm->decode_linear(bytes.value(), addr, count);
    c->insn_text.reserve(insns.size());
    c->insn_rows.reserve(insns.size());
    for (const auto& insn : insns) {
        c->insn_text.push_back(insn.text);
        c->insn_rows.push_back(IridaInsnRow{insn.address, c->insn_text.back().c_str(), nullptr});
    }

    *out = c->insn_rows.empty() ? nullptr : c->insn_rows.data();
    return c->insn_rows.size();
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

const IridaBackendVTable kCoreVTable = {
    core_registers,   core_modules,       core_disasm,    core_run_state,   core_pc,
    core_state_epoch, core_step_into,     core_step_over, core_step_out,    core_cont,
    core_brk,         core_restart,       core_stop,      core_read_memory, core_breakpoints,
    core_bp_toggle,   core_bp_set_enabled};

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
