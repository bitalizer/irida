// SPDX-License-Identifier: BUSL-1.1
#include "backend.hpp"
#include "irida/irida.h"
#include <new>

extern "C" {

IridaSession* irida_session_create(const IridaBackendVTable* vt, void* ctx) {
    if (!vt)
        return nullptr;
    auto* s = new (std::nothrow) IridaSession{*vt, ctx};
    return s;
}

void irida_session_destroy(IridaSession* s) {
    delete s;
}

size_t irida_registers(IridaSession* s, const IridaRegister** out) {
    if (!out)
        return 0;
    if (!s || !s->vt.registers) {
        *out = nullptr;
        return 0;
    }
    return s->vt.registers(s->ctx, out);
}

size_t irida_modules(IridaSession* s, const IridaModule** out) {
    if (!out)
        return 0;
    if (!s || !s->vt.modules) {
        *out = nullptr;
        return 0;
    }
    return s->vt.modules(s->ctx, out);
}

size_t irida_disasm(IridaSession* s, uint64_t addr, size_t count, const IridaInsnRow** out) {
    if (!out)
        return 0;
    if (!s || !s->vt.disasm) {
        *out = nullptr;
        return 0;
    }
    return s->vt.disasm(s->ctx, addr, count, out);
}

} // extern "C"
