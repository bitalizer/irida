// SPDX-License-Identifier: BUSL-1.1
#include "irida/irida.h"
#include "irida/mock/mock_backend.h"
#include <cassert>
#include <cstring>

int main() {
    IridaSession* s = irida_mock_create();
    assert(s != nullptr);

    const IridaRegister* regs = nullptr;
    size_t nregs = irida_registers(s, &regs);
    assert(nregs >= 4);

    const IridaModule* mods = nullptr;
    size_t nmods = irida_modules(s, &mods);
    assert(nmods == 2);

    const IridaInsnRow* rows = nullptr;
    size_t n = irida_disasm(s, 0x1000, 4, &rows);
    assert(n >= 1);
    assert(std::strcmp(rows[0].text, "mov rcx, [rax+8]") == 0);
    assert(rows[0].annotation != nullptr);
    assert(std::strstr(rows[0].annotation, "ntdll.dll") != nullptr);

    irida_session_destroy(s);
    return 0;
}
