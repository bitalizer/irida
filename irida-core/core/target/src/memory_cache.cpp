// SPDX-License-Identifier: BUSL-1.1
#include "irida/target/memory_cache.hpp"
#include <algorithm>

namespace irida::target {

using irida::base::Result;

MemoryCache::MemoryCache(irida::backend::Backend& backend) : backend_(&backend) {}

void MemoryCache::set_epoch(uint64_t epoch) {
    epoch_ = epoch;
    prev_ = std::move(cur_);
    cur_.clear();
}

Result<std::vector<std::byte>> MemoryCache::read_page(uint64_t page_addr) {
    if (auto it = cur_.find(page_addr); it != cur_.end())
        return Result<std::vector<std::byte>>::ok(it->second);

    auto fetched = backend_->read_memory(page_addr, kPageSize);
    if (!fetched.has_value())
        return Result<std::vector<std::byte>>::err(fetched.error());

    cur_[page_addr] = fetched.value();
    return Result<std::vector<std::byte>>::ok(fetched.value());
}

Result<std::vector<std::byte>> MemoryCache::read(uint64_t addr, uint64_t len) {
    using R = Result<std::vector<std::byte>>;
    std::vector<std::byte> out;
    out.reserve(len);

    uint64_t pos = addr;
    uint64_t remaining = len;
    while (remaining > 0) {
        uint64_t page_addr = (pos / kPageSize) * kPageSize;
        uint64_t page_off = pos - page_addr;
        auto page = read_page(page_addr);
        if (!page.has_value())
            return R::err(page.error());

        uint64_t avail = kPageSize - page_off;
        uint64_t take = std::min(avail, remaining);
        const auto& bytes = page.value();
        for (uint64_t i = 0; i < take; ++i) {
            uint64_t idx = page_off + i;
            out.push_back(idx < bytes.size() ? bytes[idx] : std::byte{0});
        }
        pos += take;
        remaining -= take;
    }

    return R::ok(std::move(out));
}

bool MemoryCache::changed_since_last_epoch(uint64_t addr, uint64_t len) {
    if (len == 0)
        return false;

    uint64_t first_page = (addr / kPageSize) * kPageSize;
    uint64_t last_page = ((addr + len - 1) / kPageSize) * kPageSize;

    for (uint64_t page_addr = first_page; page_addr <= last_page; page_addr += kPageSize) {
        auto cur_page = read_page(page_addr);
        auto prev_it = prev_.find(page_addr);
        if (!cur_page.has_value())
            return false;
        if (prev_it == prev_.end())
            continue; // no previous-epoch data to compare against.
        if (cur_page.value() != prev_it->second)
            return true;
    }
    return false;
}

} // namespace irida::target
