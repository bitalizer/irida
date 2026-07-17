// SPDX-License-Identifier: BUSL-1.1
#include "irida/irida.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

constexpr size_t kRegisterPreviewCount = 8;
constexpr size_t kDisasmPreviewCount = 10;

void print_registers(IridaSession* session) {
    const IridaRegister* regs = nullptr;
    size_t n = irida_registers(session, &regs);
    size_t shown = n < kRegisterPreviewCount ? n : kRegisterPreviewCount;
    for (size_t i = 0; i < shown; ++i)
        std::printf("  %-6s = 0x%016llx\n", regs[i].name,
                    static_cast<unsigned long long>(regs[i].value));
}

void print_disasm(IridaSession* session, uint64_t addr, size_t count) {
    const IridaInsnRow* rows = nullptr;
    size_t n = irida_disasm(session, addr, count, &rows);
    for (size_t i = 0; i < n; ++i)
        std::printf("  0x%016llx: %s\n", static_cast<unsigned long long>(rows[i].address),
                    rows[i].text ? rows[i].text : "(undecoded)");
}

} // namespace

int main(int argc, char** argv) {
    uint32_t pid = 0;
    bool have_pid = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--native") == 0 && i + 1 < argc) {
            pid = static_cast<uint32_t>(std::strtoul(argv[i + 1], nullptr, 10));
            have_pid = true;
            ++i;
        }
    }

    if (!have_pid) {
        std::fprintf(stderr, "usage: irida-cli --native <pid>\n");
        return 1;
    }

    IridaSession* session = irida_session_create_native(pid);
    if (!session) {
        std::fprintf(stderr, "irida-cli: irida_session_create_native failed (unsupported platform, "
                             "bad pid, or attach error)\n");
        return 1;
    }

    uint64_t pc = irida_pc(session);
    std::printf("pc = 0x%016llx\n", static_cast<unsigned long long>(pc));

    std::printf("registers:\n");
    print_registers(session);

    std::printf("disasm @ pc:\n");
    print_disasm(session, pc, kDisasmPreviewCount);

    irida_step_into(session);

    uint64_t new_pc = irida_pc(session);
    std::printf("pc after step_into = 0x%016llx\n", static_cast<unsigned long long>(new_pc));
    std::printf("disasm @ new pc:\n");
    print_disasm(session, new_pc, 1);

    irida_session_destroy_native(session);
    return 0;
}
