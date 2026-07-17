// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include "irida/backend/types.hpp"
#include "irida/base/bytes.hpp"
#include "irida/base/result.hpp"
#include <cstddef>
#include <span>
#include <variant>
#include <vector>

namespace irida::backend {

// Abstract debug backend. ONE interface, many transports (gdb/qemu, native, hypervisor).
// Everything above (Target, eval, GUI) depends ONLY on this — never a concrete transport.
class Backend {
  public:
    virtual ~Backend() = default;

    // life
    virtual irida::base::Result<std::monostate> attach(const AttachSpec&) = 0;
    virtual irida::base::Result<std::monostate> detach() = 0;

    // flow — return a normalized StopReply (our type, not the wire's)
    virtual irida::base::Result<StopReply> cont() = 0;
    virtual irida::base::Result<StopReply> step() = 0;
    virtual irida::base::Result<std::monostate> request_stop() = 0; // async halt request

    // state
    virtual irida::base::Result<irida::base::Bytes> read_registers() = 0;
    virtual irida::base::Result<std::monostate>
    write_registers(std::span<const std::byte> block) = 0;
    virtual const RegisterProfile& register_profile() const = 0;

    // memory
    virtual irida::base::Result<irida::base::Bytes> read_memory(uint64_t addr, uint64_t len) = 0;
    virtual irida::base::Result<std::monostate> write_memory(uint64_t addr,
                                                             std::span<const std::byte> data) = 0;
    virtual irida::base::Result<std::vector<MemMap>> maps() = 0;    // may be empty
    virtual irida::base::Result<std::vector<Module>> modules() = 0; // may be empty

    // breakpoints
    virtual irida::base::Result<std::monostate> set_breakpoint(BpKind kind, uint64_t addr,
                                                               int size) = 0;
    virtual irida::base::Result<std::monostate> remove_breakpoint(BpKind kind, uint64_t addr,
                                                                  int size) = 0;

    // capabilities
    virtual BackendCaps caps() const = 0;
};

} // namespace irida::backend
