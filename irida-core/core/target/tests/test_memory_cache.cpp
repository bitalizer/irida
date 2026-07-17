// SPDX-License-Identifier: BUSL-1.1
//
// Drives MemoryCache through an injected MockBackend instead of a scripted
// gdb server/GdbClient — MemoryCache now reads through irida::backend::Backend,
// not GdbClient directly, so this test exercises that same seam Target uses.
#include "irida/target/memory_cache.hpp"
#include "mock_backend.hpp"
#include <cassert>

using irida::backend::MockBackend;
using irida::target::MemoryCache;

namespace {
std::vector<std::byte> page_of(unsigned char fill, uint64_t size = 4096) {
    return std::vector<std::byte>(static_cast<size_t>(size), std::byte{fill});
}
} // namespace

int main() {
    MockBackend backend;
    // Page containing 0x1000 initially reads as pattern 0xAA, then flips to 0xBB.
    backend.set_memory(0x1000, page_of(0xAA));

    MemoryCache cache(backend);
    cache.set_epoch(1);

    // First read of this page fetches from the (mock) backend: pattern 0xAA.
    auto first = cache.read(0x1000, 16);
    assert(first.has_value());
    assert(first.value().size() == 16);
    for (auto b : first.value())
        assert(b == std::byte{0xAA});

    // Second read of the same address within the same epoch is a cache hit
    // (the backend would now return something different if asked again,
    // since we haven't flipped yet the value is still 0xAA either way, but
    // this exercises the cache path without a second fetch changing the
    // outcome).
    auto second = cache.read(0x1000, 16);
    assert(second.has_value());
    assert(second.value() == first.value());

    // No previous epoch recorded yet for this page (epoch 1 is the first),
    // so there's nothing stale to diff against.
    assert(!cache.changed_since_last_epoch(0x1000, 16));

    // Simulate a guest stop: bump the epoch (rotates cur_ -> prev_) and make
    // the backend return different bytes for subsequent memory reads
    // (self-modifying code).
    backend.set_memory(0x1000, page_of(0xBB));
    cache.set_epoch(2);

    // changed_since_last_epoch triggers a fresh read for the current epoch
    // and compares it against the rotated-in previous-epoch page.
    assert(cache.changed_since_last_epoch(0x1000, 16));

    auto third = cache.read(0x1000, 16);
    assert(third.has_value());
    for (auto b : third.value())
        assert(b == std::byte{0xBB});

    return 0;
}
