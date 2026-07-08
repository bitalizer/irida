// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/base/result.hpp"
#include "irida/proto/packets.hpp"
#include "irida/target/guest_state.hpp"
#include "irida/target/memory_cache.hpp"
#include "irida/target/register_set.hpp"
#include "irida/transport/gdb_client.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace irida::target {

// The orchestrator: owns a GdbClient plus the current GuestState, drives
// run/step/stop, and bumps the stop-epoch on every halt (re-reading
// registers, invalidating cached memory). refresh_state() is the single
// place the epoch advances.
class Target {
  public:
    static irida::base::Result<Target> attach(std::string_view host, uint16_t port);

    Target(Target&&) noexcept = default;
    Target& operator=(Target&&) noexcept = default;
    Target(const Target&) = delete;
    Target& operator=(const Target&) = delete;

    uint64_t epoch() const;
    const RegisterSet& registers() const;
    irida::base::Result<std::vector<std::byte>> read_memory(uint64_t addr, uint64_t len);
    irida::base::Result<irida::proto::StopReply> cont();
    irida::base::Result<irida::proto::StopReply> step();
    bool memory_changed(uint64_t addr, uint64_t len);

  private:
    friend class irida::base::Result<Target>;
    Target();

    // Re-reads registers, bumps epoch_, rotates the memory cache, and
    // rebuilds state_. The single place the stop-epoch advances.
    irida::base::Result<std::monostate> refresh_state();

    std::unique_ptr<irida::transport::GdbClient> client_;
    std::unique_ptr<MemoryCache> cache_;
    uint64_t epoch_ = 0;
    std::unique_ptr<GuestState> state_;
};

} // namespace irida::target
