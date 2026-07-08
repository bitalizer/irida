// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <atomic>
#include <cstdint>
#include <thread>

// A test-only in-process gdb-remote stub for exercising MemoryCache's
// epoch-diff behavior. Unlike MockGdbServer (which always replies with a
// fixed "deadbeef" page), this server flips the byte pattern it returns for
// `m` (read-memory) requests once `flip()` is called, so a test can:
//   1. read a page (gets pattern A),
//   2. call flip(),
//   3. read the same page again in a new epoch (gets pattern B),
// and assert the cache detects the difference. `g`/`c`/`s`/`?` behave like
// MockGdbServer; `Z`/`z`/`M` reply "OK".
class ScriptedGdbServer {
  public:
    ScriptedGdbServer();
    ~ScriptedGdbServer();
    uint16_t port() const {
        return port_;
    }

    // After this call, subsequent `m` replies return the "flipped" page
    // pattern instead of the initial one.
    void flip();

  private:
    void run();
    uint16_t port_ = 0;
    intptr_t listen_sock_ = -1;
    std::thread thread_;
    std::atomic<bool> flipped_{false};
};
