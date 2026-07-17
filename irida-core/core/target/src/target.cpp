// SPDX-License-Identifier: BUSL-1.1
#include "irida/target/target.hpp"
#include <utility>

namespace irida::target {

using irida::base::Result;
namespace proto = irida::proto;
namespace backend = irida::backend;

Target::Target() = default;

// NOTE: Target::attach(host, port) — the GdbBackend convenience overload — lives in its
// own translation unit, target_gdb_attach.cpp. It calls a forward-declared
// irida::backend::make_gdb_backend(), which is implemented in a separate lane
// (core/backend/src/gdb_backend.cpp, not present on this branch). Keeping it in a
// separate .cpp means the static-lib .obj containing the unresolved reference is only
// pulled into a link when that overload is actually used — so every test/executable on
// this branch, which only exercises attach(unique_ptr<Backend>), links standalone
// without needing make_gdb_backend() to exist yet.
// TODO(merge): make_gdb_backend() provided by gdb-impl lane.

Result<Target> Target::attach(std::unique_ptr<backend::Backend> new_backend) {
    using R = Result<Target>;

    Target target;
    target.backend_ = std::move(new_backend);
    target.cache_ = std::make_unique<MemoryCache>(*target.backend_);

    if (auto r = target.refresh_state(); !r.has_value())
        return R::err(r.error());

    return R::ok(std::move(target));
}

Result<std::monostate> Target::refresh_state() {
    using R = Result<std::monostate>;

    auto raw_regs = backend_->read_registers();
    if (!raw_regs.has_value())
        return R::err(raw_regs.error());

    auto decoded = RegisterSet::decode(raw_regs.value());
    if (!decoded.has_value())
        return R::err(decoded.error());

    ++epoch_;
    cache_->set_epoch(epoch_);
    state_ = std::make_unique<GuestState>(epoch_, decoded.value(), *cache_);

    return R::ok(std::monostate{});
}

namespace {
// Target::cont()/step() keep returning irida::proto::StopReply (the wire type) to avoid
// churning callers/ABI; Backend::cont()/step() return backend::StopReply (our normalized
// type). This is the small reverse-map back to the wire shape the plan calls for.
proto::StopReply to_proto_stop_reply(const backend::StopReply& s) {
    proto::StopReply out;
    switch (s.kind) {
    case backend::StopKind::Breakpoint:
    case backend::StopKind::SingleStep:
    case backend::StopKind::Signal:
        out.kind = proto::StopReply::Signal;
        break;
    case backend::StopKind::Exited:
        out.kind = proto::StopReply::Exited;
        break;
    case backend::StopKind::Error:
    case backend::StopKind::Unknown:
        out.kind = proto::StopReply::Unknown;
        break;
    }
    out.code = s.signal;
    out.output = s.detail;
    return out;
}
} // namespace

uint64_t Target::epoch() const {
    return epoch_;
}

const RegisterSet& Target::registers() const {
    return state_->registers();
}

Result<std::vector<std::byte>> Target::read_memory(uint64_t addr, uint64_t len) {
    return state_->read(addr, len);
}

Result<proto::StopReply> Target::cont() {
    using R = Result<proto::StopReply>;
    auto stop = backend_->cont();
    if (!stop.has_value())
        return R::err(stop.error());
    if (auto r = refresh_state(); !r.has_value())
        return R::err(r.error());
    return R::ok(to_proto_stop_reply(stop.value()));
}

Result<proto::StopReply> Target::step() {
    using R = Result<proto::StopReply>;
    auto stop = backend_->step();
    if (!stop.has_value())
        return R::err(stop.error());
    if (auto r = refresh_state(); !r.has_value())
        return R::err(r.error());
    return R::ok(to_proto_stop_reply(stop.value()));
}

bool Target::memory_changed(uint64_t addr, uint64_t len) {
    return cache_->changed_since_last_epoch(addr, len);
}

} // namespace irida::target
