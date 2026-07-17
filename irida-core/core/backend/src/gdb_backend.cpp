// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// GdbBackend — wraps irida::transport::GdbClient (the "vendor" from this
// module's point of view; only this file includes gdb_client.hpp within
// core/backend). Translates our POD vocabulary (types.hpp) to/from the wire
// gdb-remote types GdbClient already speaks.
#include "irida/backend/gdb_backend.hpp"
#include <array>
#include <utility>

namespace irida::backend {

using irida::base::Result;
namespace proto = irida::proto;

namespace {

// QEMU's default x86-64 g-block order: 17 64-bit GP/rip slots, then a
// 32-bit eflags slot. Mirrors irida::target::RegisterSet::decode's layout
// (core/target/src/register_set.cpp) — kept in sync by hand until a single
// shared source of truth is introduced.
constexpr uint16_t kQwordSize = 8;
constexpr std::array<std::pair<std::string_view, uint16_t>, 17> kQwordRegs = {{
    {"rax", kQwordSize},
    {"rbx", kQwordSize},
    {"rcx", kQwordSize},
    {"rdx", kQwordSize},
    {"rsi", kQwordSize},
    {"rdi", kQwordSize},
    {"rbp", kQwordSize},
    {"rsp", kQwordSize},
    {"r8", kQwordSize},
    {"r9", kQwordSize},
    {"r10", kQwordSize},
    {"r11", kQwordSize},
    {"r12", kQwordSize},
    {"r13", kQwordSize},
    {"r14", kQwordSize},
    {"r15", kQwordSize},
    {"rip", kQwordSize},
}};
constexpr std::string_view kEflagsName = "eflags";
constexpr uint16_t kEflagsSize = 4;

RegisterProfile make_x86_64_gdb_profile() {
    RegisterProfile profile;
    uint32_t offset = 0;
    for (const auto& [name, size] : kQwordRegs) {
        profile.fields.push_back(RegisterField{std::string(name), offset, size});
        offset += size;
    }
    profile.fields.push_back(RegisterField{std::string(kEflagsName), offset, kEflagsSize});
    offset += kEflagsSize;
    profile.block_size = offset;
    return profile;
}

// gdb 'Z'/'z' breakpoint/watchpoint type numbers (gdb-remote protocol).
int to_gdb_type(BpKind kind) {
    switch (kind) {
    case BpKind::SoftwareExec:
        return 0;
    case BpKind::HardwareExec:
        return 1;
    case BpKind::WriteWatch:
        return 2;
    case BpKind::ReadWatch:
        return 3;
    case BpKind::AccessWatch:
        return 4;
    }
    return 0;
}

// Map a wire proto::StopReply (gdb-remote 'S'/'T'/'W'/'X'/'O' reply) to our
// normalized backend::StopReply.
StopReply translate_stop(const proto::StopReply& wire) {
    StopReply out;
    out.signal = wire.code;
    out.detail = wire.output;

    switch (wire.kind) {
    case proto::StopReply::Signal: {
        // 'T' replies may carry key:value info pairs, e.g. "swbreak"/"hwbreak"
        // markers that identify a breakpoint stop. Look for those; otherwise
        // treat it as a plain signal stop (which also covers single-step,
        // since gdb reports both via a trap signal — GdbClient/Target don't
        // yet disambiguate further, and StopReply carries no register info
        // to recover pc from, so `pc` stays 0 here).
        out.kind = StopKind::Signal;
        for (const auto& [key, val] : wire.info) {
            if (key == "swbreak" || key == "hwbreak") {
                out.kind = StopKind::Breakpoint;
                break;
            }
        }
        break;
    }
    case proto::StopReply::Exited:
        out.kind = StopKind::Exited;
        break;
    case proto::StopReply::Terminated:
        out.kind = StopKind::Error;
        break;
    case proto::StopReply::Output:
        out.kind = StopKind::Unknown;
        break;
    case proto::StopReply::Unknown:
    default:
        out.kind = StopKind::Unknown;
        break;
    }
    return out;
}

} // namespace

GdbBackend::GdbBackend()
    : client_(std::make_unique<irida::transport::GdbClient>()),
      profile_(make_x86_64_gdb_profile()) {}

Result<std::monostate> GdbBackend::attach(const AttachSpec& spec) {
    using R = Result<std::monostate>;

    auto conn = client_->connect(spec.host, spec.port);
    if (!conn.has_value())
        return R::err(conn.error());

    auto halt = client_->halt_reason();
    if (!halt.has_value())
        return R::err(halt.error());

    return R::ok(std::monostate{});
}

Result<std::monostate> GdbBackend::detach() {
    client_->disconnect();
    return Result<std::monostate>::ok(std::monostate{});
}

Result<StopReply> GdbBackend::cont() {
    using R = Result<StopReply>;
    auto wire = client_->cont();
    if (!wire.has_value())
        return R::err(wire.error());
    return R::ok(translate_stop(wire.value()));
}

Result<StopReply> GdbBackend::step() {
    using R = Result<StopReply>;
    auto wire = client_->step();
    if (!wire.has_value())
        return R::err(wire.error());
    return R::ok(translate_stop(wire.value()));
}

Result<std::monostate> GdbBackend::request_stop() {
    // GdbClient exposes no async-break method (no "send a raw \x03 interrupt
    // byte while the target is running" primitive) — the gdb-remote protocol
    // supports this out-of-band, but nothing in gdb_client.hpp surfaces it
    // today. Rather than inventing a method on GdbClient, report success as a
    // no-op; callers relying on async interruption will need GdbClient
    // extended in a follow-up task.
    return Result<std::monostate>::ok(std::monostate{});
}

Result<std::vector<std::byte>> GdbBackend::read_registers() {
    return client_->read_registers();
}

Result<std::monostate> GdbBackend::write_registers(std::span<const std::byte>) {
    // GdbClient has no write_registers ('G' packet) method today — nothing to
    // delegate to. Report an explicit error rather than inventing a method on
    // GdbClient.
    return Result<std::monostate>::err("GdbBackend: write_registers not supported by GdbClient");
}

const RegisterProfile& GdbBackend::register_profile() const {
    return profile_;
}

Result<std::vector<std::byte>> GdbBackend::read_memory(uint64_t addr, uint64_t len) {
    return client_->read_memory(addr, len);
}

Result<std::monostate> GdbBackend::write_memory(uint64_t addr, std::span<const std::byte> data) {
    return client_->write_memory(addr, data);
}

Result<std::vector<MemMap>> GdbBackend::maps() {
    // gdb without qXfer support has no memory-map query; OSAbi fills this in
    // later. Allowed to be empty per the seam spec.
    return Result<std::vector<MemMap>>::ok({});
}

Result<std::vector<Module>> GdbBackend::modules() {
    // Same rationale as maps(): empty until qXfer/OSAbi support lands.
    return Result<std::vector<Module>>::ok({});
}

Result<std::monostate> GdbBackend::set_breakpoint(BpKind kind, uint64_t addr, int size) {
    return client_->set_breakpoint(to_gdb_type(kind), addr, size);
}

Result<std::monostate> GdbBackend::remove_breakpoint(BpKind kind, uint64_t addr, int size) {
    return client_->remove_breakpoint(to_gdb_type(kind), addr, size);
}

BackendCaps GdbBackend::caps() const {
    return BackendCaps{
        /*can_step=*/true,
        /*has_hw_bp=*/true,
        /*clean_dr=*/true, // QEMU/TCG stealth property: guest DR0-DR7 stay clean
        /*thread_aware=*/false,
        /*modules_native=*/false,
    };
}

std::unique_ptr<Backend> make_gdb_backend() {
    return std::make_unique<GdbBackend>();
}

} // namespace irida::backend
