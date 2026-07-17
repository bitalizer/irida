// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// MockBackend — an in-memory Backend test double. Lets code above the seam
// (Target, eval, ...) be exercised with canned register/memory data and a
// scripted stop sequence, without a real gdb/native transport.
#pragma once
#include "irida/backend/backend.hpp"
#include "irida/base/bytes.hpp"
#include <cstring>
#include <map>
#include <queue>

namespace irida::backend {

class MockBackend final : public Backend {
  public:
    // ---- test setup helpers -------------------------------------------------
    void set_registers(irida::base::Bytes block) {
        registers_ = std::move(block);
    }
    void set_profile(RegisterProfile profile) {
        profile_ = std::move(profile);
    }
    // Plant bytes at an address so read_memory() serves them.
    void set_memory(uint64_t addr, irida::base::Bytes bytes) {
        memory_[addr] = std::move(bytes);
    }
    // Queue a stop-reply to be returned by the next cont()/step(). If empty,
    // cont()/step() default to a SingleStep stop.
    void push_stop(StopReply reply) {
        stops_.push(std::move(reply));
    }
    void set_caps(BackendCaps caps) {
        caps_ = caps;
    }

    // ---- Backend interface --------------------------------------------------
    irida::base::Result<std::monostate> attach(const AttachSpec&) override {
        return irida::base::Result<std::monostate>::ok(std::monostate{});
    }
    irida::base::Result<std::monostate> detach() override {
        return irida::base::Result<std::monostate>::ok(std::monostate{});
    }

    irida::base::Result<StopReply> cont() override {
        return next_stop();
    }
    irida::base::Result<StopReply> step() override {
        return next_stop();
    }
    irida::base::Result<std::monostate> request_stop() override {
        return irida::base::Result<std::monostate>::ok(std::monostate{});
    }

    irida::base::Result<irida::base::Bytes> read_registers() override {
        return irida::base::Result<irida::base::Bytes>::ok(registers_);
    }
    irida::base::Result<std::monostate> write_registers(std::span<const std::byte> block) override {
        registers_.assign(block.begin(), block.end());
        return irida::base::Result<std::monostate>::ok(std::monostate{});
    }
    const RegisterProfile& register_profile() const override {
        return profile_;
    }

    irida::base::Result<irida::base::Bytes> read_memory(uint64_t addr, uint64_t len) override {
        // Serve from any planted region that fully contains [addr, addr+len).
        for (const auto& [base, bytes] : memory_) {
            if (addr >= base && addr + len <= base + bytes.size()) {
                auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(addr - base);
                return irida::base::Result<irida::base::Bytes>::ok(
                    irida::base::Bytes(begin, begin + static_cast<std::ptrdiff_t>(len)));
            }
        }
        return irida::base::Result<irida::base::Bytes>::err("mock: no memory at address");
    }
    irida::base::Result<std::monostate> write_memory(uint64_t addr,
                                                     std::span<const std::byte> data) override {
        auto& region = memory_[addr];
        region.assign(data.begin(), data.end());
        return irida::base::Result<std::monostate>::ok(std::monostate{});
    }
    irida::base::Result<std::vector<MemMap>> maps() override {
        return irida::base::Result<std::vector<MemMap>>::ok({});
    }
    irida::base::Result<std::vector<Module>> modules() override {
        return irida::base::Result<std::vector<Module>>::ok({});
    }

    irida::base::Result<std::monostate> set_breakpoint(BpKind, uint64_t, int) override {
        return irida::base::Result<std::monostate>::ok(std::monostate{});
    }
    irida::base::Result<std::monostate> remove_breakpoint(BpKind, uint64_t, int) override {
        return irida::base::Result<std::monostate>::ok(std::monostate{});
    }

    BackendCaps caps() const override {
        return caps_;
    }

  private:
    irida::base::Result<StopReply> next_stop() {
        StopReply reply;
        if (!stops_.empty()) {
            reply = stops_.front();
            stops_.pop();
        } else {
            reply.kind = StopKind::SingleStep;
        }
        return irida::base::Result<StopReply>::ok(reply);
    }

    irida::base::Bytes registers_;
    RegisterProfile profile_;
    std::map<uint64_t, irida::base::Bytes> memory_;
    std::queue<StopReply> stops_;
    BackendCaps caps_{true, true, true, false, false};
};

} // namespace irida::backend
