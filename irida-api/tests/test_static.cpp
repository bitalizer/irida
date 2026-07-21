// SPDX-License-Identifier: BUSL-1.1
#include "irida/irida.h"
#include <cassert>
#include <cstdint>

int main() {
    // A static session over a real system PE serves sections, exports, and
    // functions straight from the file — no process attached.
    IridaSession* s = irida_session_create_file("C:\\Windows\\System32\\kernel32.dll");
    assert(s != nullptr);

    const IridaSection* sections = nullptr;
    size_t nsec = irida_sections(s, &sections);
    assert(nsec > 0);

    const IridaExport* exports = nullptr;
    size_t nexp = irida_exports(s, &exports);
    assert(nexp > 0); // kernel32 exports many symbols

    // The synthetic module reports the file as one module at its image base.
    const IridaModule* modules = nullptr;
    size_t nmod = irida_modules(s, &modules);
    assert(nmod == 1);
    assert(modules[0].base != 0);

    // Disassembly reads bytes from the file image at a code address. Use the
    // first executable section's virtual address as a code start.
    uint64_t code_addr = 0;
    for (size_t i = 0; i < nsec; ++i) {
        if (sections[i].perms & 0x4) { // executable
            code_addr = sections[i].vaddr;
            break;
        }
    }
    assert(code_addr != 0);
    const IridaInsnRow* rows = nullptr;
    size_t ninsn = irida_disasm(s, code_addr, 4, &rows);
    assert(ninsn > 0);
    assert(rows[0].address == code_addr);

    // Analysis runs on open; the entry point yields at least one function.
    const IridaFunction* fns = nullptr;
    size_t nfn = irida_functions(s, &fns);
    assert(nfn > 0);

    // Live-only queries are inert for a static session.
    const IridaRegister* regs = nullptr;
    assert(irida_registers(s, &regs) == 0);
    const IridaThread* threads = nullptr;
    assert(irida_threads(s, &threads) == 0);

    // A bad path returns NULL rather than crashing.
    IridaSession* bad = irida_session_create_file("C:\\no\\such\\file.bin");
    assert(bad == nullptr);

    irida_session_destroy_static(s);
    return 0;
}
