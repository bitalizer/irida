// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// Smoke test: the MockBackend test double serves canned registers/memory and a
// scripted stop sequence through the Backend interface. Proves the seam is real.
#include "mock_backend.hpp"
#include <cassert>
#include <cstddef>

using irida::backend::MockBackend;
using irida::backend::StopKind;
using irida::backend::StopReply;

static std::vector<std::byte> bytes_of(std::initializer_list<int> vals) {
    std::vector<std::byte> out;
    for (int v : vals)
        out.push_back(static_cast<std::byte>(v));
    return out;
}

int main() {
    MockBackend mock;

    // Canned registers round-trip.
    mock.set_registers(bytes_of({0xDE, 0xAD, 0xBE, 0xEF}));
    auto regs = mock.read_registers();
    assert(regs.has_value());
    assert(regs.value().size() == 4);
    assert(regs.value()[0] == static_cast<std::byte>(0xDE));

    // Planted memory is served for a contained range.
    mock.set_memory(0x1000, bytes_of({0x11, 0x22, 0x33, 0x44, 0x55}));
    auto mem = mock.read_memory(0x1001, 2);
    assert(mem.has_value());
    assert(mem.value().size() == 2);
    assert(mem.value()[0] == static_cast<std::byte>(0x22));
    assert(mem.value()[1] == static_cast<std::byte>(0x33));

    // Out-of-range read errors.
    auto miss = mock.read_memory(0x9000, 4);
    assert(!miss.has_value());

    // Scripted stop, then default stop.
    mock.push_stop(StopReply{StopKind::Breakpoint, 0, 0x1234, "planted"});
    auto s1 = mock.step();
    assert(s1.has_value());
    assert(s1.value().kind == StopKind::Breakpoint);
    assert(s1.value().pc == 0x1234);

    auto s2 = mock.cont();
    assert(s2.has_value());
    assert(s2.value().kind == StopKind::SingleStep); // default when queue empty

    return 0;
}
