// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include "irida/base/result.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace irida::host {

// Platform-neutral view of a debug event (translated from DEBUG_EVENT on Windows).
enum class DbgEventKind { Breakpoint, SingleStep, Exception, ExitProcess, LoadModule, Other };
struct DbgEvent {
    DbgEventKind kind = DbgEventKind::Other;
    uint32_t thread_id = 0;
    uint64_t address = 0; // exception/bp address if applicable
    uint32_t code = 0;    // native exception code (informational)
};

struct HostMemMap {
    uint64_t start, end;
    uint8_t perms;
    std::string name;
};
struct HostModule {
    uint64_t base, size;
    std::string name;
};

// Raw x86-64 register block in a fixed layout; NativeDebuggee converts Win32 CONTEXT
// to/from this so callers never see CONTEXT. 32 little-endian 8-byte slots in order:
// rax,rbx,rcx,rdx,rsi,rdi,rbp,rsp,r8..r15,rip,rflags,cs,ss,ds,es,fs,gs,dr0..dr3,dr6,dr7.

// The only class that touches the Win32 debug APIs.
class NativeDebuggee {
  public:
    static irida::base::Result<NativeDebuggee> attach(uint32_t pid);
    ~NativeDebuggee(); // detaches if still attached
    NativeDebuggee(NativeDebuggee&&) noexcept;
    NativeDebuggee& operator=(NativeDebuggee&&) noexcept;

    irida::base::Result<std::monostate> detach();

    // Blocks until the next debug event, returns it, and leaves the debuggee stopped.
    // Caller must call resume()/resume_single_step() to continue.
    irida::base::Result<DbgEvent> wait_event(int timeout_ms);
    irida::base::Result<std::monostate> resume();             // ContinueDebugEvent, DBG_CONTINUE
    irida::base::Result<std::monostate> resume_single_step(); // set TF, then continue
    irida::base::Result<std::monostate> request_stop();       // DebugBreakProcess

    irida::base::Result<std::vector<std::byte>> read_registers(uint32_t thread_id);
    irida::base::Result<std::monostate> write_registers(uint32_t thread_id,
                                                        std::span<const std::byte> block);

    irida::base::Result<std::vector<std::byte>> read_memory(uint64_t addr, uint64_t len);
    irida::base::Result<std::monostate> write_memory(uint64_t addr,
                                                     std::span<const std::byte> data);

    irida::base::Result<std::vector<HostMemMap>> maps();    // VirtualQueryEx walk
    irida::base::Result<std::vector<HostModule>> modules(); // EnumProcessModulesEx

    // Hardware breakpoints via DR0-DR3 + DR7 in the thread CONTEXT.
    irida::base::Result<std::monostate> set_hw_exec_bp(int slot, uint64_t addr);
    irida::base::Result<std::monostate> clear_hw_bp(int slot);
    // Software breakpoint = write 0xCC, remember original byte (NativeBackend tracks the map).

    uint32_t primary_thread() const; // first/last-stopped thread id

  private:
    friend class irida::base::Result<NativeDebuggee>;
    NativeDebuggee();
    struct Impl; // pImpl hides HANDLE/DEBUG_EVENT/etc.
    std::unique_ptr<Impl> impl_;
};

} // namespace irida::host
