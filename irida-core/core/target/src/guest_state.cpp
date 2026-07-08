// SPDX-License-Identifier: BUSL-1.1
#include "irida/target/guest_state.hpp"
#include <utility>

namespace irida::target {

GuestState::GuestState(uint64_t epoch, RegisterSet registers, MemoryCache& cache)
    : epoch_(epoch), registers_(std::move(registers)), cache_(&cache) {}

uint64_t GuestState::epoch() const {
    return epoch_;
}

const RegisterSet& GuestState::registers() const {
    return registers_;
}

irida::base::Result<std::vector<std::byte>> GuestState::read(uint64_t addr, uint64_t len) {
    return cache_->read(addr, len);
}

} // namespace irida::target
