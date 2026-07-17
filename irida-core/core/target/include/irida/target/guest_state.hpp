// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/base/bytes.hpp"
#include "irida/base/result.hpp"
#include "irida/target/memory_cache.hpp"
#include "irida/target/register_set.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace irida::target {

// An immutable-ish snapshot of registers + memory for exactly one
// stop-epoch. Owned by Target and replaced wholesale on every epoch bump;
// memory reads are served (and cached) lazily through the shared
// MemoryCache.
class GuestState {
  public:
    GuestState(uint64_t epoch, RegisterSet registers, MemoryCache& cache);

    uint64_t epoch() const;
    const RegisterSet& registers() const;
    irida::base::Result<irida::base::Bytes> read(uint64_t addr, uint64_t len);

  private:
    uint64_t epoch_;
    RegisterSet registers_;
    MemoryCache* cache_;
};

} // namespace irida::target
