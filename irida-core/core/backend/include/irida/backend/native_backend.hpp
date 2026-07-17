// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include "irida/backend/backend.hpp"
#include "irida/base/bytes.hpp"
#include "irida/host/native_debuggee.hpp"
#include <map>
#include <memory>
#include <optional>

namespace irida::backend {

class NativeBackend final : public Backend {
  public:
    NativeBackend();
    irida::base::Result<std::monostate> attach(const AttachSpec&) override;
    irida::base::Result<std::monostate> detach() override;
    irida::base::Result<StopReply> cont() override;
    irida::base::Result<StopReply> step() override;
    irida::base::Result<std::monostate> request_stop() override;
    irida::base::Result<irida::base::Bytes> read_registers() override;
    irida::base::Result<std::monostate> write_registers(std::span<const std::byte>) override;
    const RegisterProfile& register_profile() const override;
    irida::base::Result<irida::base::Bytes> read_memory(uint64_t, uint64_t) override;
    irida::base::Result<std::monostate> write_memory(uint64_t, std::span<const std::byte>) override;
    irida::base::Result<std::vector<MemMap>> maps() override;
    irida::base::Result<std::vector<Module>> modules() override;
    irida::base::Result<std::vector<ThreadInfo>> threads() override;
    irida::base::Result<std::monostate> set_breakpoint(BpKind, uint64_t, int) override;
    irida::base::Result<std::monostate> remove_breakpoint(BpKind, uint64_t, int) override;
    BackendCaps caps() const override;

  private:
    irida::base::Result<StopReply> translate_stop_and_rearm(irida::host::DbgEvent event);

    std::optional<irida::host::NativeDebuggee> dbg_;
    RegisterProfile profile_;
    std::map<uint64_t, std::byte> sw_bp_orig_;
    int next_hw_slot_ = 0;
    std::map<uint64_t, int> hw_bp_slot_;
};

std::unique_ptr<Backend> make_native_backend();

} // namespace irida::backend
