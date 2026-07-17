// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#include "irida/backend/native_backend.hpp"
#include "irida/base/bytes.hpp"

namespace irida::backend {

#ifdef _WIN32

std::unique_ptr<Backend> make_native_backend() {
    return std::make_unique<NativeBackend>();
}

#else

namespace {

// Stand-in Backend for platforms with no native debug facade. Every call
// reports the same error so callers get a Result rather than a null deref.
class UnsupportedNativeBackend final : public Backend {
  public:
    irida::base::Result<std::monostate> attach(const AttachSpec&) override {
        return irida::base::Result<std::monostate>::err("native backend: unsupported platform");
    }
    irida::base::Result<std::monostate> detach() override {
        return irida::base::Result<std::monostate>::err("native backend: unsupported platform");
    }
    irida::base::Result<StopReply> cont() override {
        return irida::base::Result<StopReply>::err("native backend: unsupported platform");
    }
    irida::base::Result<StopReply> step() override {
        return irida::base::Result<StopReply>::err("native backend: unsupported platform");
    }
    irida::base::Result<std::monostate> request_stop() override {
        return irida::base::Result<std::monostate>::err("native backend: unsupported platform");
    }
    irida::base::Result<irida::base::Bytes> read_registers() override {
        return irida::base::Result<irida::base::Bytes>::err("native backend: unsupported platform");
    }
    irida::base::Result<std::monostate> write_registers(std::span<const std::byte>) override {
        return irida::base::Result<std::monostate>::err("native backend: unsupported platform");
    }
    const RegisterProfile& register_profile() const override {
        return profile_;
    }
    irida::base::Result<irida::base::Bytes> read_memory(uint64_t, uint64_t) override {
        return irida::base::Result<irida::base::Bytes>::err("native backend: unsupported platform");
    }
    irida::base::Result<std::monostate> write_memory(uint64_t,
                                                     std::span<const std::byte>) override {
        return irida::base::Result<std::monostate>::err("native backend: unsupported platform");
    }
    irida::base::Result<std::vector<MemMap>> maps() override {
        return irida::base::Result<std::vector<MemMap>>::err(
            "native backend: unsupported platform");
    }
    irida::base::Result<std::vector<Module>> modules() override {
        return irida::base::Result<std::vector<Module>>::err(
            "native backend: unsupported platform");
    }
    irida::base::Result<std::vector<ThreadInfo>> threads() override {
        return irida::base::Result<std::vector<ThreadInfo>>::err(
            "native backend: unsupported platform");
    }
    irida::base::Result<std::monostate> set_breakpoint(BpKind, uint64_t, int) override {
        return irida::base::Result<std::monostate>::err("native backend: unsupported platform");
    }
    irida::base::Result<std::monostate> remove_breakpoint(BpKind, uint64_t, int) override {
        return irida::base::Result<std::monostate>::err("native backend: unsupported platform");
    }
    BackendCaps caps() const override {
        return BackendCaps{};
    }

  private:
    RegisterProfile profile_;
};

} // namespace

std::unique_ptr<Backend> make_native_backend() {
    return std::make_unique<UnsupportedNativeBackend>();
}

#endif

} // namespace irida::backend
