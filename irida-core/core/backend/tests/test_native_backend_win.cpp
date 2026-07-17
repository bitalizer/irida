// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// Windows-only integration smoke test: spawns the helper debuggee under a job
// object (guarantees it dies even if this test aborts), attaches to it with
// NativeBackend directly (no C ABI — Target/ABI wiring isn't built yet),
// reads registers/modules/memory, then detaches and lets the job kill it.
// All crash/assert dialogs are disabled up front so a failure prints to
// stderr and exits non-zero instead of hanging on a modal popup.
#include "irida/backend/native_backend.hpp"
#include <crtdbg.h>
#include <cstdio>
#include <windows.h>

using irida::backend::AttachMethod;
using irida::backend::AttachSpec;
using irida::backend::Backend;
using irida::backend::make_native_backend;

namespace {

void disable_crash_dialogs() {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
}

// RAII guard: detaches the backend (if attached) and tears down the helper
// process + its job object on every exit path, including early "check"
// failures below.
class TestGuard {
  public:
    Backend* backend = nullptr;
    bool attached = false;
    HANDLE job = nullptr;
    HANDLE process = nullptr;
    HANDLE thread = nullptr;

    ~TestGuard() {
        if (backend != nullptr && attached) {
            (void)backend->detach();
        }
        if (process != nullptr) {
            TerminateProcess(process, 0);
            CloseHandle(process);
        }
        if (thread != nullptr) {
            CloseHandle(thread);
        }
        if (job != nullptr) {
            CloseHandle(job); // JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE kills the helper here too
        }
    }
};

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond);             \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

} // namespace

int main() {
    disable_crash_dialogs();

    TestGuard guard;

    guard.job = CreateJobObjectA(nullptr, nullptr);
    CHECK(guard.job != nullptr);

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    CHECK(SetInformationJobObject(guard.job, JobObjectExtendedLimitInformation, &limits,
                                  sizeof(limits)) != 0);

    STARTUPINFOA startup_info{};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info{};

    BOOL created = CreateProcessA(IRIDA_NATIVE_HELPER_PATH, nullptr, nullptr, nullptr, FALSE,
                                  CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr,
                                  &startup_info, &process_info);
    CHECK(created != 0);
    guard.process = process_info.hProcess;
    guard.thread = process_info.hThread;

    CHECK(AssignProcessToJobObject(guard.job, guard.process) != 0);
    ResumeThread(guard.thread);

    DWORD pid = process_info.dwProcessId;

    std::unique_ptr<Backend> backend = make_native_backend();
    CHECK(backend != nullptr);
    guard.backend = backend.get();

    AttachSpec spec;
    spec.method = AttachMethod::NativeProcess;
    spec.pid = pid;
    auto attached = backend->attach(spec);
    CHECK(attached.has_value());
    guard.attached = true;

    auto regs = backend->read_registers();
    CHECK(regs.has_value());
    CHECK(!regs.value().empty());

    auto mods = backend->modules();
    CHECK(mods.has_value());
    CHECK(!mods.value().empty());

    uint64_t base = mods.value().front().base;
    auto mem = backend->read_memory(base, 8);
    CHECK(mem.has_value());
    CHECK(mem.value().size() == 8);

    auto detached = backend->detach();
    CHECK(detached.has_value());
    guard.attached = false;

    return 0;
}
