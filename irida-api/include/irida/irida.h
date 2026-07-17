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
    const char* hint; /* dereferenced preview, may be NULL (render-only) */
} IridaRegister;
typedef struct IridaModule {
    const char* name;
    uint64_t base;
    uint64_t size;
} IridaModule;
typedef struct IridaMemMap {
    uint64_t start;
    uint64_t end;
    uint8_t perms;
    const char* name;
} IridaMemMap;
typedef struct IridaThread {
    uint32_t tid;
    uint64_t pc;
    int current; /* 0/1 */
} IridaThread;
typedef struct IridaInsnRow {
    uint64_t address;
    const char* text;       /* e.g. "mov rcx, [rax+8]" */
    const char* annotation; /* e.g. "\"C:\\...\\ntdll.dll\""; may be NULL */
} IridaInsnRow;

typedef enum IridaRunState { IRIDA_STOPPED = 0, IRIDA_RUNNING = 1 } IridaRunState;

typedef enum IridaBpType { IRIDA_BP_SOFTWARE = 0, IRIDA_BP_HARDWARE = 1 } IridaBpType;

typedef struct IridaBreakpoint {
    uint64_t address;
    int enabled; /* 0/1 */
    int type;    /* IridaBpType */
} IridaBreakpoint;

/* Backend vtable: a backend (mock now, core later) fills these. */
typedef struct IridaBackendVTable {
    size_t (*registers)(void* ctx, const IridaRegister** out);
    size_t (*modules)(void* ctx, const IridaModule** out);
    size_t (*maps)(void* ctx, const IridaMemMap** out);
    size_t (*threads)(void* ctx, const IridaThread** out);
    size_t (*disasm)(void* ctx, uint64_t addr, size_t count, const IridaInsnRow** out);
    IridaRunState (*run_state)(void* ctx);
    uint64_t (*pc)(void* ctx);
    uint64_t (*state_epoch)(void* ctx);
    void (*step_into)(void* ctx);
    void (*step_over)(void* ctx);
    void (*step_out)(void* ctx);
    void (*cont)(void* ctx);
    void (*brk)(void* ctx);
    void (*restart)(void* ctx);
    void (*stop)(void* ctx);
    size_t (*read_memory)(void* ctx, uint64_t addr, uint8_t* buf, size_t len);
    size_t (*breakpoints)(void* ctx, const IridaBreakpoint** out);
    void (*bp_toggle)(void* ctx, uint64_t addr);
    void (*bp_set_enabled)(void* ctx, uint64_t addr, int enabled);
} IridaBackendVTable;

IridaSession* irida_session_create(const IridaBackendVTable* vt, void* ctx);
void irida_session_destroy(IridaSession* s);

/* Attaches to a real running process by pid and returns a session backed by
 * a real irida::target::Target (real registers/memory/disasm/stepping).
 * Returns NULL if the platform has no native attach support or attach
 * fails. Sessions created this way own core state and MUST be destroyed
 * with irida_session_destroy_native(), not irida_session_destroy(). */
IridaSession* irida_session_create_native(uint32_t pid);
void irida_session_destroy_native(IridaSession* s);

size_t irida_registers(IridaSession* s, const IridaRegister** out);
size_t irida_modules(IridaSession* s, const IridaModule** out);
size_t irida_maps(IridaSession* s, const IridaMemMap** out);
size_t irida_threads(IridaSession* s, const IridaThread** out);
size_t irida_disasm(IridaSession* s, uint64_t addr, size_t count, const IridaInsnRow** out);

IridaRunState irida_run_state(IridaSession* s);
uint64_t irida_pc(IridaSession* s);
uint64_t irida_state_epoch(IridaSession* s);
void irida_step_into(IridaSession* s);
void irida_step_over(IridaSession* s);
void irida_step_out(IridaSession* s);
void irida_continue(IridaSession* s);
void irida_break(IridaSession* s);
void irida_restart(IridaSession* s);
void irida_stop(IridaSession* s);
size_t irida_read_memory(IridaSession* s, uint64_t addr, uint8_t* buf, size_t len);
size_t irida_breakpoints(IridaSession* s, const IridaBreakpoint** out);
void irida_bp_toggle(IridaSession* s, uint64_t addr);
void irida_bp_set_enabled(IridaSession* s, uint64_t addr, int enabled);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* IRIDA_IRIDA_H */
