// SPDX-License-Identifier: BUSL-1.1
//
// Drives Target through an injected MockBackend instead of a mock gdb
// server/GdbClient. This keeps core/target buildable+testable with zero
// dependency on make_gdb_backend() (implemented in a separate lane). The
// same canned g-block/memory/stop-reply bytes the old mock gdb server used
// are fed through MockBackend so the identical RegisterSet::decode and
// Target reverse-mapping paths are exercised.
#include "irida/backend/backend.hpp"
#include "irida/target/target.hpp"
#include "mock_backend.hpp"
#include <cassert>
#include <memory>

using irida::backend::MockBackend;
using irida::backend::StopKind;
using irida::backend::StopReply;
using irida::target::Target;

namespace {
std::vector<std::byte> bytes_from(std::initializer_list<unsigned char> vals) {
    std::vector<std::byte> out;
    out.reserve(vals.size());
    for (auto v : vals)
        out.push_back(std::byte{v});
    return out;
}

// MemoryCache always fetches through Backend a whole page (4096 bytes) at a
// time, page-aligned. MockBackend::read_memory only serves a request that is
// fully contained within a single planted region, so the planted region for
// any address MemoryCache will touch must cover its containing page.
constexpr uint64_t kPageSize = 4096;
std::vector<std::byte> page_with_prefix(std::initializer_list<unsigned char> prefix) {
    std::vector<std::byte> page(kPageSize, std::byte{0});
    size_t i = 0;
    for (auto v : prefix)
        page[i++] = std::byte{v};
    return page;
}
} // namespace

int main() {
    auto mock = std::make_unique<MockBackend>();

    // Same 8 bytes the old mock gdb server's `g` reply decoded from ("0102030405060708"):
    // one 64-bit little-endian slot -> rax.
    mock->set_registers(bytes_from({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}));

    // Same 4 bytes the old mock gdb server's `m` reply decoded from ("deadbeef"), at the
    // start of the (page-aligned) page MemoryCache will fetch.
    mock->set_memory(0x1000, page_with_prefix({0xde, 0xad, 0xbe, 0xef}));

    // Same stop the old mock gdb server sent for `s` ("S05" -> Signal, code 5).
    StopReply signal_stop;
    signal_stop.kind = StopKind::Signal;
    signal_stop.signal = 5;
    mock->push_stop(signal_stop);

    auto attached = Target::attach(std::move(mock));
    assert(attached.has_value());
    Target target = std::move(attached).value();

    // attach(): refresh_state() -> epoch becomes 1.
    assert(target.epoch() == 1);

    auto rax = target.registers().get("rax");
    assert(rax.has_value());
    assert(*rax == 0x0807060504030201ULL);

    auto mem = target.read_memory(0x1000, 4);
    assert(mem.has_value());
    assert(mem.value().size() == 4);
    assert(mem.value()[0] == std::byte{0xde});

    // step(): MockBackend returns the scripted Signal/5 stop, then
    // refresh_state() bumps epoch again; Target::step() reverse-maps
    // backend::StopReply -> proto::StopReply (the wire type it still returns).
    auto stop = target.step();
    assert(stop.has_value());
    assert(stop.value().kind == irida::proto::StopReply::Signal);
    assert(stop.value().code == 5);
    assert(target.epoch() == 2);

    return 0;
}
