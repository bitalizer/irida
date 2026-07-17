// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#include "irida/host/native_debuggee.hpp"

#ifdef _WIN32
#include <windows.h>

#include <debugapi.h>
#include <psapi.h>
#else
#error "irida::host::NativeDebuggee currently supports Windows only"
#endif

#include <cstring>

namespace irida::host {

namespace {

// Our fixed x86-64 register block layout (all slots are 8-byte little-endian,
// even the nominally-32-bit segment/eflags registers, for a uniform stride).
// Order: rax,rbx,rcx,rdx,rsi,rdi,rbp,rsp,r8..r15,rip,rflags,cs,ss,ds,es,fs,gs,
//        dr0,dr1,dr2,dr3,dr6,dr7 padded to dr0..dr7 (8 slots).
// Total: 16 GPR + rip + rflags + 6 segment + 8 dr = 32 slots * 8 bytes = 256 bytes.
constexpr size_t kNumSlots = 32;
constexpr size_t kRegBlockSize = kNumSlots * sizeof(uint64_t);

void put_slot(std::vector<std::byte>& block, size_t slot_index, uint64_t v) {
    std::memcpy(block.data() + (slot_index * sizeof(uint64_t)), &v, sizeof(uint64_t));
}

uint64_t get_slot(std::span<const std::byte> block, size_t slot_index) {
    uint64_t v = 0;
    std::memcpy(&v, block.data() + (slot_index * sizeof(uint64_t)), sizeof(uint64_t));
    return v;
}

// Serializes a CONTEXT into our fixed register block (documented order above).
std::vector<std::byte> context_to_block(const CONTEXT& ctx) {
    std::vector<std::byte> block(kRegBlockSize, std::byte{0});
    size_t i = 0;
    put_slot(block, i++, ctx.Rax);
    put_slot(block, i++, ctx.Rbx);
    put_slot(block, i++, ctx.Rcx);
    put_slot(block, i++, ctx.Rdx);
    put_slot(block, i++, ctx.Rsi);
    put_slot(block, i++, ctx.Rdi);
    put_slot(block, i++, ctx.Rbp);
    put_slot(block, i++, ctx.Rsp);
    put_slot(block, i++, ctx.R8);
    put_slot(block, i++, ctx.R9);
    put_slot(block, i++, ctx.R10);
    put_slot(block, i++, ctx.R11);
    put_slot(block, i++, ctx.R12);
    put_slot(block, i++, ctx.R13);
    put_slot(block, i++, ctx.R14);
    put_slot(block, i++, ctx.R15);
    put_slot(block, i++, ctx.Rip);
    put_slot(block, i++, static_cast<uint64_t>(ctx.EFlags));
    put_slot(block, i++, static_cast<uint64_t>(ctx.SegCs));
    put_slot(block, i++, static_cast<uint64_t>(ctx.SegSs));
    put_slot(block, i++, static_cast<uint64_t>(ctx.SegDs));
    put_slot(block, i++, static_cast<uint64_t>(ctx.SegEs));
    put_slot(block, i++, static_cast<uint64_t>(ctx.SegFs));
    put_slot(block, i++, static_cast<uint64_t>(ctx.SegGs));
    put_slot(block, i++, ctx.Dr0);
    put_slot(block, i++, ctx.Dr1);
    put_slot(block, i++, ctx.Dr2);
    put_slot(block, i++, ctx.Dr3);
    put_slot(block, i++, ctx.Dr6);
    put_slot(block, i++, ctx.Dr7);
    put_slot(block, i++, 0); // reserved (dr slot padding)
    put_slot(block, i++, 0); // reserved (dr slot padding)
    return block;
}

// Deserializes our fixed register block back into a CONTEXT (both directions
// must agree on the slot order above).
void block_to_context(std::span<const std::byte> block, CONTEXT& ctx) {
    size_t i = 0;
    ctx.Rax = get_slot(block, i++);
    ctx.Rbx = get_slot(block, i++);
    ctx.Rcx = get_slot(block, i++);
    ctx.Rdx = get_slot(block, i++);
    ctx.Rsi = get_slot(block, i++);
    ctx.Rdi = get_slot(block, i++);
    ctx.Rbp = get_slot(block, i++);
    ctx.Rsp = get_slot(block, i++);
    ctx.R8 = get_slot(block, i++);
    ctx.R9 = get_slot(block, i++);
    ctx.R10 = get_slot(block, i++);
    ctx.R11 = get_slot(block, i++);
    ctx.R12 = get_slot(block, i++);
    ctx.R13 = get_slot(block, i++);
    ctx.R14 = get_slot(block, i++);
    ctx.R15 = get_slot(block, i++);
    ctx.Rip = get_slot(block, i++);
    ctx.EFlags = static_cast<DWORD>(get_slot(block, i++));
    ctx.SegCs = static_cast<WORD>(get_slot(block, i++));
    ctx.SegSs = static_cast<WORD>(get_slot(block, i++));
    ctx.SegDs = static_cast<WORD>(get_slot(block, i++));
    ctx.SegEs = static_cast<WORD>(get_slot(block, i++));
    ctx.SegFs = static_cast<WORD>(get_slot(block, i++));
    ctx.SegGs = static_cast<WORD>(get_slot(block, i++));
    ctx.Dr0 = get_slot(block, i++);
    ctx.Dr1 = get_slot(block, i++);
    ctx.Dr2 = get_slot(block, i++);
    ctx.Dr3 = get_slot(block, i++);
    ctx.Dr6 = get_slot(block, i++);
    ctx.Dr7 = get_slot(block, i++);
    // remaining 2 reserved slots ignored on write-back.
}

std::string format_last_error(const char* what) {
    DWORD err = GetLastError();
    return std::string(what) + " failed (GetLastError=" + std::to_string(err) + ")";
}

constexpr DWORD kEflagsTrapFlag = 0x100;

} // namespace

using R0 = irida::base::Result<std::monostate>;
using RAttach = irida::base::Result<NativeDebuggee>;
using REvent = irida::base::Result<DbgEvent>;
using RRegs = irida::base::Result<std::vector<std::byte>>;
using RMem = irida::base::Result<std::vector<std::byte>>;
using RMaps = irida::base::Result<std::vector<HostMemMap>>;
using RMods = irida::base::Result<std::vector<HostModule>>;

struct NativeDebuggee::Impl {
    DWORD pid = 0;
    HANDLE process = nullptr;
    bool attached = false;

    // Pending debug event, stashed between wait_event() and resume()/resume_single_step().
    bool has_pending = false;
    DEBUG_EVENT pending{};

    // Last thread the debuggee stopped in; used as the "primary" thread and to
    // continue the correct event.
    DWORD primary_tid = 0;

    ~Impl() {
        if (process != nullptr) {
            CloseHandle(process);
            process = nullptr;
        }
    }
};

NativeDebuggee::NativeDebuggee() : impl_(std::make_unique<Impl>()) {}

NativeDebuggee::NativeDebuggee(NativeDebuggee&&) noexcept = default;
NativeDebuggee& NativeDebuggee::operator=(NativeDebuggee&&) noexcept = default;

NativeDebuggee::~NativeDebuggee() {
    if (impl_ && impl_->attached) {
        (void)detach();
    }
}

RAttach NativeDebuggee::attach(uint32_t pid) {
    if (DebugActiveProcess(static_cast<DWORD>(pid)) == 0) {
        return RAttach::err(format_last_error("DebugActiveProcess"));
    }
    // Leave the target alive if we exit without an explicit detach.
    DebugSetProcessKillOnExit(FALSE);

    HANDLE process = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
                                     PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD,
                                 FALSE, static_cast<DWORD>(pid));
    if (process == nullptr) {
        DebugActiveProcessStop(static_cast<DWORD>(pid));
        return RAttach::err(format_last_error("OpenProcess"));
    }

    NativeDebuggee dbg;
    dbg.impl_->pid = static_cast<DWORD>(pid);
    dbg.impl_->process = process;
    dbg.impl_->attached = true;
    return RAttach::ok(std::move(dbg));
}

R0 NativeDebuggee::detach() {
    if (!impl_->attached) {
        return R0::err("detach: not attached");
    }
    if (DebugActiveProcessStop(impl_->pid) == 0) {
        return R0::err(format_last_error("DebugActiveProcessStop"));
    }
    impl_->attached = false;
    return R0::ok(std::monostate{});
}

REvent NativeDebuggee::wait_event(int timeout_ms) {
    if (!impl_->attached) {
        return REvent::err("wait_event: not attached");
    }
    DEBUG_EVENT de{};
    if (WaitForDebugEvent(&de, static_cast<DWORD>(timeout_ms)) == 0) {
        return REvent::err(format_last_error("WaitForDebugEvent"));
    }

    impl_->has_pending = true;
    impl_->pending = de;
    impl_->primary_tid = de.dwThreadId;

    DbgEvent out;
    out.thread_id = de.dwThreadId;

    switch (de.dwDebugEventCode) {
    case EXCEPTION_DEBUG_EVENT: {
        const EXCEPTION_RECORD& rec = de.u.Exception.ExceptionRecord;
        out.code = rec.ExceptionCode;
        out.address = reinterpret_cast<uint64_t>(rec.ExceptionAddress);
        if (rec.ExceptionCode == static_cast<DWORD>(EXCEPTION_BREAKPOINT)) {
            out.kind = DbgEventKind::Breakpoint;
        } else if (rec.ExceptionCode == static_cast<DWORD>(EXCEPTION_SINGLE_STEP)) {
            out.kind = DbgEventKind::SingleStep;
        } else {
            out.kind = DbgEventKind::Exception;
        }
        break;
    }
    case EXIT_PROCESS_DEBUG_EVENT:
        out.kind = DbgEventKind::ExitProcess;
        out.code = de.u.ExitProcess.dwExitCode;
        break;
    case LOAD_DLL_DEBUG_EVENT:
        out.kind = DbgEventKind::LoadModule;
        out.address = reinterpret_cast<uint64_t>(de.u.LoadDll.lpBaseOfDll);
        break;
    default:
        out.kind = DbgEventKind::Other;
        break;
    }

    return REvent::ok(out);
}

R0 NativeDebuggee::resume() {
    if (!impl_->has_pending) {
        return R0::err("resume: no pending debug event");
    }
    DWORD continue_status = (impl_->pending.dwDebugEventCode == EXCEPTION_DEBUG_EVENT &&
                             impl_->pending.u.Exception.ExceptionRecord.ExceptionCode !=
                                 static_cast<DWORD>(EXCEPTION_BREAKPOINT) &&
                             impl_->pending.u.Exception.ExceptionRecord.ExceptionCode !=
                                 static_cast<DWORD>(EXCEPTION_SINGLE_STEP))
                                ? static_cast<DWORD>(DBG_EXCEPTION_NOT_HANDLED)
                                : static_cast<DWORD>(DBG_CONTINUE);

    if (ContinueDebugEvent(impl_->pending.dwProcessId, impl_->pending.dwThreadId,
                           continue_status) == 0) {
        return R0::err(format_last_error("ContinueDebugEvent"));
    }
    impl_->has_pending = false;
    return R0::ok(std::monostate{});
}

R0 NativeDebuggee::resume_single_step() {
    if (!impl_->has_pending) {
        return R0::err("resume_single_step: no pending debug event");
    }

    DWORD tid = impl_->pending.dwThreadId;
    HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, tid);
    if (thread == nullptr) {
        return R0::err(format_last_error("OpenThread"));
    }

    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(thread, &ctx) == 0) {
        CloseHandle(thread);
        return R0::err(format_last_error("GetThreadContext"));
    }

    ctx.EFlags |= kEflagsTrapFlag;

    if (SetThreadContext(thread, &ctx) == 0) {
        CloseHandle(thread);
        return R0::err(format_last_error("SetThreadContext"));
    }
    CloseHandle(thread);

    if (ContinueDebugEvent(impl_->pending.dwProcessId, impl_->pending.dwThreadId,
                           static_cast<DWORD>(DBG_CONTINUE)) == 0) {
        return R0::err(format_last_error("ContinueDebugEvent"));
    }
    impl_->has_pending = false;
    return R0::ok(std::monostate{});
}

R0 NativeDebuggee::request_stop() {
    if (!impl_->attached) {
        return R0::err("request_stop: not attached");
    }
    if (DebugBreakProcess(impl_->process) == 0) {
        return R0::err(format_last_error("DebugBreakProcess"));
    }
    return R0::ok(std::monostate{});
}

RRegs NativeDebuggee::read_registers(uint32_t thread_id) {
    HANDLE thread = OpenThread(THREAD_GET_CONTEXT, FALSE, static_cast<DWORD>(thread_id));
    if (thread == nullptr) {
        return RRegs::err(format_last_error("OpenThread"));
    }

    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(thread, &ctx) == 0) {
        CloseHandle(thread);
        return RRegs::err(format_last_error("GetThreadContext"));
    }
    CloseHandle(thread);

    return RRegs::ok(context_to_block(ctx));
}

R0 NativeDebuggee::write_registers(uint32_t thread_id, std::span<const std::byte> block) {
    if (block.size() < kRegBlockSize) {
        return R0::err("write_registers: block too small");
    }

    HANDLE thread =
        OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, static_cast<DWORD>(thread_id));
    if (thread == nullptr) {
        return R0::err(format_last_error("OpenThread"));
    }

    // Read first so we start from a fully-populated CONTEXT (preserves any
    // fields our fixed block does not carry), then overwrite our known slots.
    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(thread, &ctx) == 0) {
        CloseHandle(thread);
        return R0::err(format_last_error("GetThreadContext"));
    }

    block_to_context(block, ctx);

    if (SetThreadContext(thread, &ctx) == 0) {
        CloseHandle(thread);
        return R0::err(format_last_error("SetThreadContext"));
    }
    CloseHandle(thread);
    return R0::ok(std::monostate{});
}

RMem NativeDebuggee::read_memory(uint64_t addr, uint64_t len) {
    if (!impl_->attached) {
        return RMem::err("read_memory: not attached");
    }
    std::vector<std::byte> buf(static_cast<size_t>(len));
    SIZE_T bytes_read = 0;
    if (ReadProcessMemory(impl_->process, reinterpret_cast<LPCVOID>(addr), buf.data(),
                          static_cast<SIZE_T>(len), &bytes_read) == 0) {
        return RMem::err(format_last_error("ReadProcessMemory"));
    }
    if (bytes_read != static_cast<SIZE_T>(len)) {
        return RMem::err("read_memory: partial read");
    }
    return RMem::ok(std::move(buf));
}

R0 NativeDebuggee::write_memory(uint64_t addr, std::span<const std::byte> data) {
    if (!impl_->attached) {
        return R0::err("write_memory: not attached");
    }
    SIZE_T bytes_written = 0;
    if (WriteProcessMemory(impl_->process, reinterpret_cast<LPVOID>(addr), data.data(),
                           static_cast<SIZE_T>(data.size()), &bytes_written) == 0) {
        return R0::err(format_last_error("WriteProcessMemory"));
    }
    if (bytes_written != static_cast<SIZE_T>(data.size())) {
        return R0::err("write_memory: partial write");
    }
    if (FlushInstructionCache(impl_->process, reinterpret_cast<LPCVOID>(addr),
                              static_cast<SIZE_T>(data.size())) == 0) {
        return R0::err(format_last_error("FlushInstructionCache"));
    }
    return R0::ok(std::monostate{});
}

RMaps NativeDebuggee::maps() {
    if (!impl_->attached) {
        return RMaps::err("maps: not attached");
    }
    std::vector<HostMemMap> out;
    uint64_t addr = 0;
    MEMORY_BASIC_INFORMATION mbi{};
    while (VirtualQueryEx(impl_->process, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) ==
           sizeof(mbi)) {
        if (mbi.State == MEM_COMMIT) {
            HostMemMap m{};
            m.start = reinterpret_cast<uint64_t>(mbi.BaseAddress);
            m.end = m.start + static_cast<uint64_t>(mbi.RegionSize);

            uint8_t perms = 0;
            DWORD prot = mbi.Protect & 0xFF; // strip PAGE_GUARD/PAGE_NOCACHE modifier bits
            switch (prot) {
            case PAGE_READONLY:
                perms = 0x4; // R
                break;
            case PAGE_READWRITE:
            case PAGE_WRITECOPY:
                perms = 0x6; // RW
                break;
            case PAGE_EXECUTE:
                perms = 0x1; // X
                break;
            case PAGE_EXECUTE_READ:
                perms = 0x5; // RX
                break;
            case PAGE_EXECUTE_READWRITE:
            case PAGE_EXECUTE_WRITECOPY:
                perms = 0x7; // RWX
                break;
            default:
                perms = 0;
                break;
            }
            m.perms = perms;
            out.push_back(std::move(m));
        }

        uint64_t next =
            reinterpret_cast<uint64_t>(mbi.BaseAddress) + static_cast<uint64_t>(mbi.RegionSize);
        if (next <= addr) {
            break; // guard against non-advancing regions
        }
        addr = next;
    }
    return RMaps::ok(std::move(out));
}

RMods NativeDebuggee::modules() {
    if (!impl_->attached) {
        return RMods::err("modules: not attached");
    }

    std::vector<HMODULE> mods(256);
    DWORD needed = 0;
    if (EnumProcessModulesEx(impl_->process, mods.data(),
                             static_cast<DWORD>(mods.size() * sizeof(HMODULE)), &needed,
                             LIST_MODULES_ALL) == 0) {
        return RMods::err(format_last_error("EnumProcessModulesEx"));
    }
    size_t count = needed / sizeof(HMODULE);
    if (count > mods.size()) {
        mods.resize(count);
        if (EnumProcessModulesEx(impl_->process, mods.data(),
                                 static_cast<DWORD>(mods.size() * sizeof(HMODULE)), &needed,
                                 LIST_MODULES_ALL) == 0) {
            return RMods::err(format_last_error("EnumProcessModulesEx"));
        }
        count = needed / sizeof(HMODULE);
    }

    std::vector<HostModule> out;
    out.reserve(count);
    for (size_t idx = 0; idx < count; ++idx) {
        HMODULE h = mods[idx];
        MODULEINFO info{};
        if (GetModuleInformation(impl_->process, h, &info, sizeof(info)) == 0) {
            continue;
        }
        char name[MAX_PATH] = {};
        DWORD name_len =
            GetModuleFileNameExA(impl_->process, h, name, static_cast<DWORD>(sizeof(name)));

        HostModule m{};
        m.base = reinterpret_cast<uint64_t>(info.lpBaseOfDll);
        m.size = static_cast<uint64_t>(info.SizeOfImage);
        m.name = name_len > 0 ? std::string(name, name_len) : std::string();
        out.push_back(std::move(m));
    }
    return RMods::ok(std::move(out));
}

R0 NativeDebuggee::set_hw_exec_bp(int slot, uint64_t addr) {
    if (slot < 0 || slot > 3) {
        return R0::err("set_hw_exec_bp: slot out of range (0-3)");
    }
    if (!impl_->attached) {
        return R0::err("set_hw_exec_bp: not attached");
    }

    HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, impl_->primary_tid);
    if (thread == nullptr) {
        return R0::err(format_last_error("OpenThread"));
    }

    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(thread, &ctx) == 0) {
        CloseHandle(thread);
        return R0::err(format_last_error("GetThreadContext"));
    }

    switch (slot) {
    case 0:
        ctx.Dr0 = addr;
        break;
    case 1:
        ctx.Dr1 = addr;
        break;
    case 2:
        ctx.Dr2 = addr;
        break;
    case 3:
        ctx.Dr3 = addr;
        break;
    default:
        break;
    }

    // Dr7: enable local bit for this slot (bit 2*slot), condition = 00 (execute),
    // length = 00 (1 byte) in the corresponding RWn/LENn nibble (bits 16+4*slot).
    DWORD64 enable_bit = static_cast<DWORD64>(1) << (2 * slot);
    DWORD64 rw_len_shift = 16 + (4 * static_cast<DWORD64>(slot));
    DWORD64 rw_len_mask = static_cast<DWORD64>(0xF) << rw_len_shift;

    ctx.Dr7 &= ~rw_len_mask; // condition=00 (execute), length=00 (1 byte)
    ctx.Dr7 |= enable_bit;

    if (SetThreadContext(thread, &ctx) == 0) {
        CloseHandle(thread);
        return R0::err(format_last_error("SetThreadContext"));
    }
    CloseHandle(thread);
    return R0::ok(std::monostate{});
}

R0 NativeDebuggee::clear_hw_bp(int slot) {
    if (slot < 0 || slot > 3) {
        return R0::err("clear_hw_bp: slot out of range (0-3)");
    }
    if (!impl_->attached) {
        return R0::err("clear_hw_bp: not attached");
    }

    HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, impl_->primary_tid);
    if (thread == nullptr) {
        return R0::err(format_last_error("OpenThread"));
    }

    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(thread, &ctx) == 0) {
        CloseHandle(thread);
        return R0::err(format_last_error("GetThreadContext"));
    }

    DWORD64 enable_bit = static_cast<DWORD64>(1) << (2 * slot);
    ctx.Dr7 &= ~enable_bit;

    switch (slot) {
    case 0:
        ctx.Dr0 = 0;
        break;
    case 1:
        ctx.Dr1 = 0;
        break;
    case 2:
        ctx.Dr2 = 0;
        break;
    case 3:
        ctx.Dr3 = 0;
        break;
    default:
        break;
    }

    if (SetThreadContext(thread, &ctx) == 0) {
        CloseHandle(thread);
        return R0::err(format_last_error("SetThreadContext"));
    }
    CloseHandle(thread);
    return R0::ok(std::monostate{});
}

uint32_t NativeDebuggee::primary_thread() const {
    return static_cast<uint32_t>(impl_->primary_tid);
}

} // namespace irida::host
