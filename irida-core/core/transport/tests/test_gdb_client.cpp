// SPDX-License-Identifier: BUSL-1.1
#include "irida/transport/gdb_client.hpp"
#include "mock_gdb_server.hpp"
#include <cassert>

using namespace irida::transport;

int main() {
    MockGdbServer server;
    GdbClient client;
    auto cr = client.connect("127.0.0.1", server.port());
    assert(cr.has_value());

    auto regs = client.read_registers();
    assert(regs.has_value() && regs.value().size() == 8);
    assert(regs.value()[0] == std::byte{0x01} && regs.value()[7] == std::byte{0x08});

    auto mem = client.read_memory(0x1000, 4);
    assert(mem.has_value() && mem.value().size() == 4);
    assert(mem.value()[0] == std::byte{0xde} && mem.value()[3] == std::byte{0xef});

    auto bp = client.set_breakpoint(0, 0x401000, 1);
    assert(bp.has_value());

    auto rm = client.remove_breakpoint(0, 0x401000, 1);
    assert(rm.has_value());

    std::byte w[] = {std::byte{0x90}};
    auto wr = client.write_memory(0x2000, {w, 1});
    assert(wr.has_value());

    auto stop = client.cont();
    assert(stop.has_value() && stop.value().kind == irida::proto::StopReply::Signal);
    assert(stop.value().code == 5);

    client.disconnect();
    return 0;
}
