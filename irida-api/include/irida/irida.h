/* SPDX-License-Identifier: BUSL-1.1
 * Copyright (c) 2026 Bitalizer.
 * Irida stable C ABI. Only opaque handles and POD structs cross this boundary. */
#ifndef IRIDA_IRIDA_H
#define IRIDA_IRIDA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IridaSession IridaSession;

typedef struct IridaRegister {
    const char* name;
    uint64_t value;
} IridaRegister;
typedef struct IridaModule {
    const char* name;
    uint64_t base;
    uint64_t size;
} IridaModule;
typedef struct IridaInsnRow {
    uint64_t address;
    const char* text;       /* e.g. "mov rcx, [rax+8]" */
    const char* annotation; /* e.g. "\"C:\\...\\ntdll.dll\""; may be NULL */
} IridaInsnRow;

/* Backend vtable: a backend (mock now, core later) fills these. */
typedef struct IridaBackendVTable {
    size_t (*registers)(void* ctx, const IridaRegister** out);
    size_t (*modules)(void* ctx, const IridaModule** out);
    size_t (*disasm)(void* ctx, uint64_t addr, size_t count, const IridaInsnRow** out);
} IridaBackendVTable;

IridaSession* irida_session_create(const IridaBackendVTable* vt, void* ctx);
void irida_session_destroy(IridaSession* s);

size_t irida_registers(IridaSession* s, const IridaRegister** out);
size_t irida_modules(IridaSession* s, const IridaModule** out);
size_t irida_disasm(IridaSession* s, uint64_t addr, size_t count, const IridaInsnRow** out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* IRIDA_IRIDA_H */
