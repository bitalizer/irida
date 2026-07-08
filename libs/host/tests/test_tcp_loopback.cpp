// SPDX-License-Identifier: BUSL-1.1
// Spins a listening socket in-process, connects TcpClient to it, and checks a
// send/recv round-trip. No external dependency; localhost only.
#include "irida/host/tcp.hpp"
#include <cassert>
#include <cstring>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

using namespace irida::host;

int main() {
#ifdef _WIN32
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);
#endif
    // Listen on an ephemeral port.
    SOCKET lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    bind(lsock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    int alen = sizeof(addr);
    getsockname(lsock, reinterpret_cast<sockaddr*>(&addr), &alen);
    uint16_t port = ntohs(addr.sin_port);
    listen(lsock, 1);

    // Server thread: accept, echo one message back.
    std::thread server([&] {
        SOCKET c = accept(lsock, nullptr, nullptr);
        char buf[16];
        int n = recv(c, buf, sizeof(buf), 0);
        send(c, buf, n, 0);
        closesocket(c);
    });

    TcpClient client;
    auto cr = client.connect("127.0.0.1", port);
    assert(cr.has_value());

    const char msg[] = "ping";
    auto sr = client.send({reinterpret_cast<const std::byte*>(msg), 4});
    assert(sr.has_value() && sr.value() == 4);

    std::byte rbuf[16];
    auto rr = client.recv({rbuf, sizeof(rbuf)}, 2000);
    assert(rr.has_value() && rr.value() == 4);
    assert(std::memcmp(rbuf, "ping", 4) == 0);

    client.close();
    server.join();
    closesocket(lsock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
