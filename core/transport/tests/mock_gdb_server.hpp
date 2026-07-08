// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <cstdint>
#include <string>
#include <thread>

// A test-only in-process gdb-remote stub. Listens on an ephemeral localhost
// port, acks every packet with '+', and replies to specific requests with
// scripted, framed responses. One client, then exits.
class MockGdbServer {
public:
    MockGdbServer();
    ~MockGdbServer();
    uint16_t port() const { return port_; }

private:
    void run();
    uint16_t port_ = 0;
    intptr_t listen_sock_ = -1;
    std::thread thread_;
};
