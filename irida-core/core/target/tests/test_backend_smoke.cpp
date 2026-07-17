// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// Injects a MockBackend and asserts epoch() advances on step() and read_memory()
// returns bytes planted via MockBackend::set_memory.
#include "irida/backend/backend.hpp"
#include "irida/base/bytes.hpp"
#include "irida/target/target.hpp"
#include "mock_backend.hpp"
#include <cassert>
#include <memory>

using irida::backend::MockBackend;
using irida::base::Bytes;
using irida::target::Target;

namespace {
// MemoryCache fetches through Backend a whole page (4096 bytes) at a time,
// page-aligned, so the planted region must cover the containing page —
// MockBackend::read_memory only serves requests fully within one planted region.
constexpr uint64_t kPageSize = 4096;
} // namespace

int main() {
    auto mock = std::make_unique<MockBackend>();

    // Minimal register block so RegisterSet::decode succeeds (refresh_state() runs on attach).
    mock->set_registers(Bytes(8, std::byte{0}));

    Bytes page(kPageSize, std::byte{0});
    page[0] = std::byte{0xCA};
    page[1] = std::byte{0xFE};
    page[2] = std::byte{0xBA};
    page[3] = std::byte{0xBE};
    mock->set_memory(0x2000, page);

    auto attached = Target::attach(std::move(mock));
    assert(attached.has_value());
    Target target = std::move(attached).value();

    uint64_t epoch_after_attach = target.epoch();

    auto stop = target.step();
    assert(stop.has_value());
    assert(target.epoch() > epoch_after_attach); // epoch() advances after step()

    auto mem = target.read_memory(0x2000, 4);
    assert(mem.has_value());
    assert(mem.value().size() == 4);
    assert(mem.value()[0] == std::byte{0xCA});
    assert(mem.value()[1] == std::byte{0xFE});
    assert(mem.value()[2] == std::byte{0xBA});
    assert(mem.value()[3] == std::byte{0xBE});

    return 0;
}
