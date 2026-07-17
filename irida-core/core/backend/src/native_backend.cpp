// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// NativeBackend — wraps irida::host::NativeDebuggee (the only third-party
// surface this file touches; no windows headers here, only the host facade).
// Translates our POD vocabulary (types.hpp) to/from irida::host's vocabulary.
#include "irida/backend/native_backend.hpp"
#include <array>
#include <cstring>
#include <utility>

namespace irida::backend {

using irida::base::Bytes;
using irida::base::Result;
namespace host = irida::host;

namespace {

constexpr std::byte kSoftwareBpOpcode{0xCC};
constexpr uint32_t kRipOffset = 16 * 8; // slot 16: rax..r15 (16 slots) precede rip
constexpr int kAttachDrainTimeoutMs = 300;

struct SlotSpec {
    std::string_view name;
    uint16_t size;
};

constexpr uint16_t kSlotSize = 8;
constexpr std::array<SlotSpec, 32> kSlots = {{
    {"rax", kSlotSize}, {"rbx", kSlotSize},    {"rcx", kSlotSize},       {"rdx", kSlotSize},
    {"rsi", kSlotSize}, {"rdi", kSlotSize},    {"rbp", kSlotSize},       {"rsp", kSlotSize},
    {"r8", kSlotSize},  {"r9", kSlotSize},     {"r10", kSlotSize},       {"r11", kSlotSize},
    {"r12", kSlotSize}, {"r13", kSlotSize},    {"r14", kSlotSize},       {"r15", kSlotSize},
    {"rip", kSlotSize}, {"rflags", kSlotSize}, {"cs", kSlotSize},        {"ss", kSlotSize},
    {"ds", kSlotSize},  {"es", kSlotSize},     {"fs", kSlotSize},        {"gs", kSlotSize},
    {"dr0", kSlotSize}, {"dr1", kSlotSize},    {"dr2", kSlotSize},       {"dr3", kSlotSize},
    {"dr6", kSlotSize}, {"dr7", kSlotSize},    {"reserved0", kSlotSize}, {"reserved1", kSlotSize},
}};

RegisterProfile make_native_x86_64_profile() {
    RegisterProfile profile;
    uint32_t offset = 0;
    for (const auto& slot : kSlots) {
        profile.fields.push_back(RegisterField{std::string(slot.name), offset, slot.size});
        offset += slot.size;
    }
    profile.block_size = offset;
    return profile;
}

void write_rip(Bytes& block, uint64_t rip) {
    if (block.size() >= kRipOffset + sizeof(uint64_t)) {
        std::memcpy(block.data() + kRipOffset, &rip, sizeof(uint64_t));
    }
}

StopKind translate_kind(host::DbgEventKind kind) {
    switch (kind) {
    case host::DbgEventKind::Breakpoint:
        return StopKind::Breakpoint;
    case host::DbgEventKind::SingleStep:
        return StopKind::SingleStep;
    case host::DbgEventKind::Exception:
        return StopKind::Signal;
    case host::DbgEventKind::ExitProcess:
        return StopKind::Exited;
    case host::DbgEventKind::LoadModule:
    case host::DbgEventKind::Other:
    default:
        return StopKind::Unknown;
    }
}

} // namespace

NativeBackend::NativeBackend() : profile_(make_native_x86_64_profile()) {}

Result<std::monostate> NativeBackend::attach(const AttachSpec& spec) {
    using R = Result<std::monostate>;

    auto attached = host::NativeDebuggee::attach(spec.pid);
    if (!attached.has_value())
        return R::err(attached.error());
    dbg_.emplace(std::move(attached).value());

    // Attaching to an already-running process makes Windows synthesize a whole
    // stream of debug events (create-process/thread, one load-module per
    // already-loaded DLL, plus loader breakpoints) before the target settles.
    // Loader-owned state (e.g. EnumProcessModulesEx) isn't reliably queryable
    // until that stream is fully drained, so continue events until none
    // arrive within the drain timeout, then force a deterministic stop.
    while (true) {
        auto event = dbg_->wait_event(kAttachDrainTimeoutMs);
        if (!event.has_value())
            break; // no more synthetic events pending; target is running free
        auto resumed = dbg_->resume();
        if (!resumed.has_value())
            return R::err(resumed.error());
    }

    auto stop_request = dbg_->request_stop();
    if (!stop_request.has_value())
        return R::err(stop_request.error());
    auto stopped = dbg_->wait_event(-1);
    if (!stopped.has_value())
        return R::err(stopped.error());

    return R::ok(std::monostate{});
}

Result<std::monostate> NativeBackend::detach() {
    using R = Result<std::monostate>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: detach called without an attached target");

    auto result = dbg_->detach();
    dbg_.reset();
    if (!result.has_value())
        return R::err(result.error());
    return R::ok(std::monostate{});
}

Result<StopReply> NativeBackend::translate_stop_and_rearm(host::DbgEvent event) {
    using R = Result<StopReply>;

    if (event.kind == host::DbgEventKind::Breakpoint) {
        auto planted = sw_bp_orig_.find(event.address);
        if (planted != sw_bp_orig_.end()) {
            // Rewind the primary thread's rip past the 0xCC we injected, restore
            // the original byte, single-step over the real instruction, then
            // re-arm the 0xCC so the breakpoint stays live for the next hit.
            std::byte original = planted->second;
            uint32_t tid = dbg_->primary_thread();

            auto regs = dbg_->read_registers(tid);
            if (!regs.has_value())
                return R::err(regs.error());
            auto block = regs.value();
            write_rip(block, event.address);
            auto set_regs = dbg_->write_registers(tid, block);
            if (!set_regs.has_value())
                return R::err(set_regs.error());

            auto restore = dbg_->write_memory(event.address, std::span(&original, 1));
            if (!restore.has_value())
                return R::err(restore.error());

            auto step_result = dbg_->resume_single_step();
            if (!step_result.has_value())
                return R::err(step_result.error());
            auto step_event = dbg_->wait_event(-1);
            if (!step_event.has_value())
                return R::err(step_event.error());

            auto rearm = dbg_->write_memory(event.address, std::span(&kSoftwareBpOpcode, 1));
            if (!rearm.has_value())
                return R::err(rearm.error());
        }
    }

    StopReply reply;
    reply.kind = translate_kind(event.kind);
    reply.signal = static_cast<int>(event.code);
    reply.pc = event.address;
    return R::ok(reply);
}

Result<StopReply> NativeBackend::cont() {
    using R = Result<StopReply>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: cont called without an attached target");

    auto resumed = dbg_->resume();
    if (!resumed.has_value())
        return R::err(resumed.error());

    auto event = dbg_->wait_event(-1);
    if (!event.has_value())
        return R::err(event.error());

    return translate_stop_and_rearm(event.value());
}

Result<StopReply> NativeBackend::step() {
    using R = Result<StopReply>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: step called without an attached target");

    auto stepped = dbg_->resume_single_step();
    if (!stepped.has_value())
        return R::err(stepped.error());

    auto event = dbg_->wait_event(-1);
    if (!event.has_value())
        return R::err(event.error());

    StopReply reply;
    reply.kind = StopKind::SingleStep;
    reply.pc = event.value().address;
    return R::ok(reply);
}

Result<std::monostate> NativeBackend::request_stop() {
    using R = Result<std::monostate>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: request_stop called without an attached target");
    return dbg_->request_stop();
}

Result<Bytes> NativeBackend::read_registers() {
    using R = Result<Bytes>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: read_registers called without an attached target");
    return dbg_->read_registers(dbg_->primary_thread());
}

Result<std::monostate> NativeBackend::write_registers(std::span<const std::byte> block) {
    using R = Result<std::monostate>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: write_registers called without an attached target");
    return dbg_->write_registers(dbg_->primary_thread(), block);
}

const RegisterProfile& NativeBackend::register_profile() const {
    return profile_;
}

Result<Bytes> NativeBackend::read_memory(uint64_t addr, uint64_t len) {
    using R = Result<Bytes>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: read_memory called without an attached target");
    return dbg_->read_memory(addr, len);
}

Result<std::monostate> NativeBackend::write_memory(uint64_t addr, std::span<const std::byte> data) {
    using R = Result<std::monostate>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: write_memory called without an attached target");
    return dbg_->write_memory(addr, data);
}

Result<std::vector<MemMap>> NativeBackend::maps() {
    using R = Result<std::vector<MemMap>>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: maps called without an attached target");

    auto host_maps = dbg_->maps();
    if (!host_maps.has_value())
        return R::err(host_maps.error());

    std::vector<MemMap> out;
    out.reserve(host_maps.value().size());
    for (const auto& m : host_maps.value()) {
        out.push_back(MemMap{m.start, m.end, m.perms, m.name});
    }
    return R::ok(std::move(out));
}

Result<std::vector<Module>> NativeBackend::modules() {
    using R = Result<std::vector<Module>>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: modules called without an attached target");

    auto host_modules = dbg_->modules();
    if (!host_modules.has_value())
        return R::err(host_modules.error());

    std::vector<Module> out;
    out.reserve(host_modules.value().size());
    for (const auto& m : host_modules.value()) {
        out.push_back(Module{m.base, m.size, m.name});
    }
    return R::ok(std::move(out));
}

Result<std::monostate> NativeBackend::set_breakpoint(BpKind kind, uint64_t addr, int size) {
    using R = Result<std::monostate>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: set_breakpoint called without an attached target");

    switch (kind) {
    case BpKind::SoftwareExec: {
        auto original = dbg_->read_memory(addr, 1);
        if (!original.has_value())
            return R::err(original.error());
        sw_bp_orig_[addr] = original.value()[0];

        auto write = dbg_->write_memory(addr, std::span(&kSoftwareBpOpcode, 1));
        if (!write.has_value())
            return R::err(write.error());
        return R::ok(std::monostate{});
    }
    case BpKind::HardwareExec: {
        if (next_hw_slot_ > 3)
            return R::err("NativeBackend: no free hardware breakpoint slot");
        int slot = next_hw_slot_++;
        auto result = dbg_->set_hw_exec_bp(slot, addr);
        if (!result.has_value())
            return R::err(result.error());
        hw_bp_slot_[addr] = slot;
        return R::ok(std::monostate{});
    }
    case BpKind::WriteWatch:
    case BpKind::ReadWatch:
    case BpKind::AccessWatch:
        (void)size;
        return R::err("watchpoint not implemented");
    }
    return R::err("NativeBackend: unknown breakpoint kind");
}

Result<std::monostate> NativeBackend::remove_breakpoint(BpKind kind, uint64_t addr, int size) {
    using R = Result<std::monostate>;
    if (!dbg_.has_value())
        return R::err("NativeBackend: remove_breakpoint called without an attached target");

    switch (kind) {
    case BpKind::SoftwareExec: {
        auto planted = sw_bp_orig_.find(addr);
        if (planted == sw_bp_orig_.end())
            return R::err("NativeBackend: no software breakpoint planted at address");
        auto restore = dbg_->write_memory(addr, std::span(&planted->second, 1));
        if (!restore.has_value())
            return R::err(restore.error());
        sw_bp_orig_.erase(planted);
        return R::ok(std::monostate{});
    }
    case BpKind::HardwareExec: {
        auto slot = hw_bp_slot_.find(addr);
        if (slot == hw_bp_slot_.end())
            return R::err("NativeBackend: no hardware breakpoint at address");
        auto result = dbg_->clear_hw_bp(slot->second);
        if (!result.has_value())
            return R::err(result.error());
        hw_bp_slot_.erase(slot);
        return R::ok(std::monostate{});
    }
    case BpKind::WriteWatch:
    case BpKind::ReadWatch:
    case BpKind::AccessWatch:
        (void)size;
        return R::err("watchpoint not implemented");
    }
    return R::err("NativeBackend: unknown breakpoint kind");
}

BackendCaps NativeBackend::caps() const {
    return BackendCaps{
        /*can_step=*/true,
        /*has_hw_bp=*/true,
        /*clean_dr=*/false,
        /*thread_aware=*/true,
        /*modules_native=*/true,
    };
}

} // namespace irida::backend
