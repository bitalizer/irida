// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/backend/backend.hpp"
#include "irida/base/bytes.hpp"
#include "irida/base/result.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace irida::target {

// Page-cached guest memory reads through a Backend, keyed to the current
// stop-epoch. Reads are satisfied from cached pages; misses fetch a
// page-aligned range via Backend::read_memory. `set_epoch` rotates the
// current generation of pages into the "previous epoch" slot so that a
// later re-read can be diffed against it (self-modification detection).
class MemoryCache {
  public:
    explicit MemoryCache(irida::backend::Backend& backend);

    // Rotates cur_ -> prev_ and clears cur_. Call this once per new epoch.
    void set_epoch(uint64_t epoch);

    irida::base::Result<irida::base::Bytes> read(uint64_t addr, uint64_t len);

    // True if any byte in [addr, addr+len) differs between the page(s) as
    // cached in the current epoch and as cached in the previous epoch. If a
    // page wasn't read (cached) in one of the two epochs, it's fetched on
    // demand so the diff is accurate.
    bool changed_since_last_epoch(uint64_t addr, uint64_t len);

  private:
    static constexpr uint64_t kPageSize = 4096;

    irida::base::Result<irida::base::Bytes> read_page(uint64_t page_addr);

    irida::backend::Backend* backend_;
    uint64_t epoch_ = 0;
    std::map<uint64_t, irida::base::Bytes> cur_;
    std::map<uint64_t, irida::base::Bytes> prev_;
};

} // namespace irida::target
