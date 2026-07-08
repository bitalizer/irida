// SPDX-License-Identifier: BUSL-1.1
#include "irida/target/target.hpp"
#include <utility>

namespace irida::target {

using irida::base::Result;
namespace proto = irida::proto;

Target::Target() = default;

Result<Target> Target::attach(std::string_view host, uint16_t port) {
    using R = Result<Target>;

    Target target;
    target.client_ = std::make_unique<irida::transport::GdbClient>();

    auto conn = target.client_->connect(host, port);
    if (!conn.has_value())
        return R::err(conn.error());

    target.cache_ = std::make_unique<MemoryCache>(*target.client_);

    auto halt = target.client_->halt_reason();
    if (!halt.has_value())
        return R::err(halt.error());

    if (auto r = target.refresh_state(); !r.has_value())
        return R::err(r.error());

    return R::ok(std::move(target));
}

Result<std::monostate> Target::refresh_state() {
    using R = Result<std::monostate>;

    auto raw_regs = client_->read_registers();
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
    auto stop = client_->cont();
    if (!stop.has_value())
        return R::err(stop.error());
    if (auto r = refresh_state(); !r.has_value())
        return R::err(r.error());
    return R::ok(stop.value());
}

Result<proto::StopReply> Target::step() {
    using R = Result<proto::StopReply>;
    auto stop = client_->step();
    if (!stop.has_value())
        return R::err(stop.error());
    if (auto r = refresh_state(); !r.has_value())
        return R::err(r.error());
    return R::ok(stop.value());
}

bool Target::memory_changed(uint64_t addr, uint64_t len) {
    return cache_->changed_since_last_epoch(addr, len);
}

} // namespace irida::target
