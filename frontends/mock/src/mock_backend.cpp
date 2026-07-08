// SPDX-License-Identifier: BUSL-1.1
#include "irida/mock/mock_backend.h"
#include <cstdint>

namespace {

// Static fake data. Pointers handed across the ABI stay valid for the process.
const IridaRegister kRegs[] = {
    {"rax", 0x00007ff6abcd0000ULL},
    {"rbx", 0x0000000000000000ULL},
    {"rcx", 0x0000000000000010ULL},
    {"rsp", 0x000000000014fe00ULL},
};

const IridaModule kMods[] = {
    {"ntdll.dll", 0x00007ff6abcd0000ULL, 0x1f0000},
    {"kernel32.dll", 0x00007ff6aa000000ULL, 0x0d0000},
};

// The planted "resolved value" is what makes the annotation column light up.
const IridaInsnRow kRows[] = {
    {0x1000, "mov rcx, [rax+8]", "\"C:\\Windows\\System32\\ntdll.dll\""},
    {0x1004, "lea rdx, [rip+0x2f1c]", "kernel32.dll+0x1a20 (CreateFileW)"},
    {0x100b, "xor eax, eax", nullptr},
    {0x100d, "call rdx", nullptr},
};

size_t mock_registers(void*, const IridaRegister** out) {
    *out = kRegs;
    return sizeof(kRegs) / sizeof(kRegs[0]);
}
size_t mock_modules(void*, const IridaModule** out) {
    *out = kMods;
    return sizeof(kMods) / sizeof(kMods[0]);
}
size_t mock_disasm(void*, uint64_t /*addr*/, size_t count, const IridaInsnRow** out) {
    *out = kRows;
    size_t total = sizeof(kRows) / sizeof(kRows[0]);
    return count < total ? count : total;
}

const IridaBackendVTable kVTable = {mock_registers, mock_modules, mock_disasm};

} // namespace

extern "C" IridaSession* irida_mock_create(void) {
    return irida_session_create(&kVTable, nullptr);
}
