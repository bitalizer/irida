// SPDX-License-Identifier: BUSL-1.1
#include "irida/target/target.hpp"
#include "mock_gdb_server.hpp"
#include <cassert>

using irida::target::Target;

int main() {
    MockGdbServer server;

    auto attached = Target::attach("127.0.0.1", server.port());
    assert(attached.has_value());
    Target target = std::move(attached).value();

    // attach(): connect -> `?` -> refresh_state() -> epoch becomes 1.
    assert(target.epoch() == 1);

    // Mock's `g` reply is "0102030405060708" (8 bytes -> one 64-bit slot: rax).
    auto rax = target.registers().get("rax");
    assert(rax.has_value());
    assert(*rax == 0x0807060504030201ULL);

    auto mem = target.read_memory(0x1000, 4);
    assert(mem.has_value());
    assert(mem.value().size() == 4);
    assert(mem.value()[0] == std::byte{0xde});

    // step(): mock replies "S05" to `s`, then refresh_state() bumps epoch again.
    auto stop = target.step();
    assert(stop.has_value());
    assert(stop.value().kind == irida::proto::StopReply::Signal);
    assert(stop.value().code == 5);
    assert(target.epoch() == 2);

    return 0;
}
