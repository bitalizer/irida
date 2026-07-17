// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// Trivial target process for test_native_backend_win.cpp: sleeps until poked
// (attached, read, detached) then exits on its own, so a crashed/aborted test
// never leaves it running forever.
#include <windows.h>

namespace {
constexpr DWORD kMaxLifetimeMs = 30000;
constexpr DWORD kTickMs = 200;
} // namespace

int main() {
    for (DWORD elapsed = 0; elapsed < kMaxLifetimeMs; elapsed += kTickMs) {
        Sleep(kTickMs);
    }
    return 0;
}
