// SPDX-License-Identifier: BUSL-1.1
#include "irida/mock/mock_backend.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

// One canned x86-64-ish instruction stream the fake PC walks. address+length
// are what the disassembly panel and stepping use; text/annotation feed the
// instruction + live-value columns. `is_call` drives step-over/step-out.
struct Insn {
    uint64_t address;
    uint32_t length;
    const char* bytes; // space-separated hex, display only
    const char* text;
    const char* annotation; // live-value; may be null
    bool is_call;
};

const Insn kStream[] = {
    {0x1000, 4, "48 8B 48 08", "mov rcx, [rax+8]", "\"C:\\Windows\\System32\\ntdll.dll\"", false},
    {0x1004, 7, "48 8D 15 1C 2F 00 00", "lea rdx, [rip+0x2f1c]",
     "kernel32.dll+0x1a20 (CreateFileW)", false},
    {0x100b, 2, "31 C0", "xor eax, eax", nullptr, false},
    {0x100d, 2, "FF D2", "call rdx", nullptr, true},
    {0x100f, 3, "48 83 C4 28", "add rsp, 0x28", nullptr, false},
    {0x1012, 1, "C3", "ret", nullptr, false},
};
constexpr size_t kStreamLen = sizeof(kStream) / sizeof(kStream[0]);

// Register indices for the fake file. rip is derived from pc.
enum Reg { RAX, RBX, RCX, RDX, RSP, RBP, RSI, RDI, R8, R9, R15, REG_COUNT };

struct MockState {
    uint64_t pc = kStream[0].address;
    uint64_t epoch = 1;
    IridaRunState run_state = IRIDA_STOPPED;
    size_t ip_index = 0; // index into kStream matching pc
    std::array<uint64_t, REG_COUNT> regs{};
    std::vector<IridaBreakpoint> bps;
    std::vector<uint8_t> mem; // backing blob for read_memory

    // Scratch kept alive for pointer-returning ABI calls.
    std::vector<IridaRegister> reg_view;
    std::vector<std::string> reg_hints;
    std::vector<IridaInsnRow> row_view;

    MockState() {
        reset_regs();
        mem.resize(0x400);
        for (size_t i = 0; i < mem.size(); ++i)
            mem[i] = static_cast<uint8_t>((i * 7 + 0x41) & 0xFF); // deterministic, printable-ish
        // plant an ASCII string near the start so the hex panel shows readable bytes
        const char* planted = "ntdll.dll";
        std::memcpy(mem.data() + 0x20, planted, std::strlen(planted));
    }

    void reset_regs() {
        regs[RAX] = 0x00007ff6abcd0000ULL;
        regs[RBX] = 0;
        regs[RCX] = 0x10;
        regs[RDX] = 0;
        regs[RSP] = 0x000000000014fe00ULL;
        regs[RBP] = 0x000000000014ff00ULL;
        regs[RSI] = 0;
        regs[RDI] = 0;
        regs[R8] = 0;
        regs[R9] = 0;
        regs[R15] = 0;
    }

    void find_index() {
        for (size_t i = 0; i < kStreamLen; ++i)
            if (kStream[i].address == pc) {
                ip_index = i;
                return;
            }
    }
};

MockState& state() {
    static MockState s;
    return s;
}

void advance_one() {
    MockState& s = state();
    s.find_index();
    // mutate a couple registers so the Registers panel visibly recolors
    s.regs[RAX] += 0x10;
    s.regs[RCX] = s.regs[RCX] + 1;
    if (s.ip_index + 1 < kStreamLen)
        s.pc = kStream[s.ip_index + 1].address;
    s.epoch++;
}

// --- vtable implementations ---
size_t mock_registers(void*, const IridaRegister** out) {
    MockState& s = state();
    static const char* names[REG_COUNT] = {"rax", "rbx", "rcx", "rdx", "rsp", "rbp",
                                           "rsi", "rdi", "r8",  "r9",  "r15"};
    s.reg_view.clear();
    s.reg_hints.assign(REG_COUNT, std::string());
    for (int i = 0; i < REG_COUNT; ++i) {
        IridaRegister r;
        r.name = names[i];
        r.value = s.regs[static_cast<size_t>(i)];
        r.hint = nullptr;
        s.reg_view.push_back(r);
    }
    // one planted hint so the Hint column is visibly populated
    s.reg_hints[RAX] = "-> module base (ntdll.dll)";
    s.reg_view[RAX].hint = s.reg_hints[RAX].c_str();
    *out = s.reg_view.data();
    return s.reg_view.size();
}

size_t mock_modules(void*, const IridaModule** out) {
    static const IridaModule mods[] = {
        {"ntdll.dll", 0x00007ff6abcd0000ULL, 0x1f0000},
        {"kernel32.dll", 0x00007ff6aa000000ULL, 0x0d0000},
    };
    *out = mods;
    return sizeof(mods) / sizeof(mods[0]);
}

size_t mock_disasm(void*, uint64_t /*addr*/, size_t count, const IridaInsnRow** out) {
    MockState& s = state();
    s.row_view.clear();
    size_t n = count < kStreamLen ? count : kStreamLen;
    for (size_t i = 0; i < n; ++i) {
        IridaInsnRow row;
        row.address = kStream[i].address;
        row.text = kStream[i].text;
        row.annotation = kStream[i].annotation;
        s.row_view.push_back(row);
    }
    *out = s.row_view.data();
    return s.row_view.size();
}

IridaRunState mock_run_state(void*) {
    return state().run_state;
}
uint64_t mock_pc(void*) {
    return state().pc;
}
uint64_t mock_epoch(void*) {
    return state().epoch;
}
void mock_step_into(void*) {
    advance_one();
}
void mock_step_over(void*) {
    MockState& s = state();
    s.find_index();
    if (kStream[s.ip_index].is_call && s.ip_index + 1 < kStreamLen) {
        s.pc = kStream[s.ip_index + 1].address; // skip over call
        s.regs[RAX] += 0x10;
        s.epoch++;
    } else {
        advance_one();
    }
}
void mock_step_out(void*) {
    MockState& s = state();
    // jump to the last instruction (fake "return to caller")
    s.pc = kStream[kStreamLen - 1].address;
    s.epoch++;
}
void mock_cont(void*) {
    MockState& s = state();
    // run to the next enabled breakpoint at a higher address, else last insn
    uint64_t target = kStream[kStreamLen - 1].address;
    uint64_t best = UINT64_MAX;
    for (const auto& b : s.bps)
        if (b.enabled && b.address > s.pc && b.address < best)
            best = b.address;
    if (best != UINT64_MAX)
        target = best;
    s.pc = target;
    s.epoch++;
}
void mock_brk(void*) {
    MockState& s = state();
    s.run_state = IRIDA_STOPPED;
    s.epoch++;
}
void mock_restart(void*) {
    MockState& s = state();
    s.pc = kStream[0].address;
    s.reset_regs();
    s.run_state = IRIDA_STOPPED;
    s.epoch++;
}
void mock_stop(void*) {
    MockState& s = state();
    s.pc = kStream[0].address;
    s.reset_regs();
    s.run_state = IRIDA_STOPPED;
    s.epoch++;
}
size_t mock_read_memory(void*, uint64_t addr, uint8_t* buf, size_t len) {
    MockState& s = state();
    // map addr into the blob by low bits; deterministic, always fills
    for (size_t i = 0; i < len; ++i)
        buf[i] = s.mem[(addr + i) % s.mem.size()];
    return len;
}
size_t mock_breakpoints(void*, const IridaBreakpoint** out) {
    MockState& s = state();
    *out = s.bps.empty() ? nullptr : s.bps.data();
    return s.bps.size();
}
void mock_bp_toggle(void*, uint64_t addr) {
    MockState& s = state();
    for (size_t i = 0; i < s.bps.size(); ++i)
        if (s.bps[i].address == addr) {
            s.bps.erase(s.bps.begin() + static_cast<long>(i));
            return;
        }
    s.bps.push_back(IridaBreakpoint{addr, 1, IRIDA_BP_SOFTWARE});
}
void mock_bp_set_enabled(void*, uint64_t addr, int enabled) {
    MockState& s = state();
    for (auto& b : s.bps)
        if (b.address == addr)
            b.enabled = enabled ? 1 : 0;
}

const IridaBackendVTable kVTable = {
    mock_registers, mock_modules,       mock_disasm,    mock_run_state,   mock_pc,
    mock_epoch,     mock_step_into,     mock_step_over, mock_step_out,    mock_cont,
    mock_brk,       mock_restart,       mock_stop,      mock_read_memory, mock_breakpoints,
    mock_bp_toggle, mock_bp_set_enabled};

} // namespace

extern "C" IridaSession* irida_mock_create(void) {
    // fresh run state each create
    state().pc = kStream[0].address;
    state().reset_regs();
    state().epoch = 1;
    state().bps.clear();
    return irida_session_create(&kVTable, nullptr);
}
