// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace irida::backend {

// How to attach — a tagged union kept trivial so the C ABI can mirror it later.
enum class AttachMethod { GdbRemote, NativeProcess };
struct AttachSpec {
    AttachMethod method = AttachMethod::GdbRemote;
    std::string host;  // GdbRemote: e.g. "127.0.0.1"
    uint16_t port = 0; // GdbRemote
    uint32_t pid = 0;  // NativeProcess
};

enum class BpKind { SoftwareExec, HardwareExec, WriteWatch, ReadWatch, AccessWatch };

struct MemMap { // one mapped region
    uint64_t start = 0;
    uint64_t end = 0;  // exclusive
    uint8_t perms = 0; // bit0=r bit1=w bit2=x
    std::string name;  // module/region name if known, else ""
};

struct Module {
    uint64_t base = 0;
    uint64_t size = 0;
    std::string name;
};

struct ThreadInfo {
    uint32_t tid = 0;
    uint64_t pc = 0;
    bool current = false;
};

// Normalized stop reason — backend translates its native/wire form into this.
enum class StopKind { Breakpoint, SingleStep, Signal, Exited, Error, Unknown };
struct StopReply {
    StopKind kind = StopKind::Unknown;
    int signal = 0;     // POSIX-ish signal number if applicable
    uint64_t pc = 0;    // pc at stop if the backend knows it, else 0
    std::string detail; // human string for the log/UI
};

// What a backend can/can't do — lets core+GUI adapt without knowing the concrete type.
struct BackendCaps {
    bool can_step = false;       // single-step supported
    bool has_hw_bp = false;      // DR0-DR7 style hardware breakpoints
    bool clean_dr = false;       // hw bps invisible to target (TCG/QEMU) — the stealth bit
    bool thread_aware = false;   // reports/selects threads
    bool modules_native = false; // backend enumerates modules itself (else OSAbi must)
};

// A register-layout descriptor the backend supplies so RegisterSet can decode uniformly
// regardless of transport (gdb 'g'-block order vs Windows CONTEXT order).
struct RegisterField {
    std::string name;    // "rax", "rip", "eflags", ...
    uint32_t offset = 0; // byte offset into the raw register block
    uint16_t size = 0;   // bytes
};
struct RegisterProfile {
    std::vector<RegisterField> fields;
    uint32_t block_size = 0; // total bytes of the raw register block
};

} // namespace irida::backend
