// SPDX-License-Identifier: BUSL-1.1
#include "irida/target/memory_cache.hpp"
#include "irida/transport/gdb_client.hpp"
#include "scripted_gdb_server.hpp"
#include <cassert>

using irida::target::MemoryCache;
using irida::transport::GdbClient;

int main() {
    ScriptedGdbServer server;
    GdbClient client;
    auto cr = client.connect("127.0.0.1", server.port());
    assert(cr.has_value());

    MemoryCache cache(client);
    cache.set_epoch(1);

    // First read of this page fetches from the (scripted) server: pattern 0xAA.
    auto first = cache.read(0x1000, 16);
    assert(first.has_value());
    assert(first.value().size() == 16);
    for (auto b : first.value())
        assert(b == std::byte{0xAA});

    // Second read of the same address within the same epoch is a cache hit
    // (server would now return something different if asked again, since we
    // haven't flipped yet the value is still 0xAA either way, but this
    // exercises the cache path without a second network fetch changing the
    // outcome).
    auto second = cache.read(0x1000, 16);
    assert(second.has_value());
    assert(second.value() == first.value());

    // No previous epoch recorded yet for this page (epoch 1 is the first),
    // so there's nothing stale to diff against.
    assert(!cache.changed_since_last_epoch(0x1000, 16));

    // Simulate a guest stop: bump the epoch (rotates cur_ -> prev_) and make
    // the server return different bytes for subsequent memory reads
    // (self-modifying code).
    server.flip();
    cache.set_epoch(2);

    // changed_since_last_epoch triggers a fresh read for the current epoch
    // and compares it against the rotated-in previous-epoch page.
    assert(cache.changed_since_last_epoch(0x1000, 16));

    auto third = cache.read(0x1000, 16);
    assert(third.has_value());
    for (auto b : third.value())
        assert(b == std::byte{0xBB});

    client.disconnect();
    return 0;
}
