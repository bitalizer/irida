// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include "irida/backend/backend.hpp"
#include "irida/transport/gdb_client.hpp"
#include <memory>

namespace irida::backend {

class GdbBackend final : public Backend {
  public:
    GdbBackend();
    // Backend interface — all methods delegate to GdbClient and translate wire↔our types.
    irida::base::Result<std::monostate> attach(const AttachSpec&) override; // uses host/port
    irida::base::Result<std::monostate> detach() override;
    irida::base::Result<StopReply> cont() override;
    irida::base::Result<StopReply> step() override;
    irida::base::Result<std::monostate> request_stop() override;
    irida::base::Result<std::vector<std::byte>> read_registers() override;
    irida::base::Result<std::monostate> write_registers(std::span<const std::byte>) override;
    const RegisterProfile& register_profile() const override;
    irida::base::Result<std::vector<std::byte>> read_memory(uint64_t, uint64_t) override;
    irida::base::Result<std::monostate> write_memory(uint64_t, std::span<const std::byte>) override;
    irida::base::Result<std::vector<MemMap>> maps() override;
    irida::base::Result<std::vector<Module>> modules() override;
    irida::base::Result<std::monostate> set_breakpoint(BpKind, uint64_t, int) override;
    irida::base::Result<std::monostate> remove_breakpoint(BpKind, uint64_t, int) override;
    BackendCaps caps() const override;

  private:
    std::unique_ptr<irida::transport::GdbClient> client_;
    RegisterProfile profile_; // x86-64 gdb 'g'-block layout, built in ctor
};

// Factory (keeps construction uniform + hides the header from callers that only want Backend).
std::unique_ptr<Backend> make_gdb_backend();

} // namespace irida::backend
