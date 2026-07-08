// SPDX-License-Identifier: BUSL-1.1
#include "mock_gdb_server.hpp"
#include "irida/proto/frame.hpp"
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

using irida::proto::frame;
using irida::proto::FrameDecoder;

MockGdbServer::MockGdbServer() {
#ifdef _WIN32
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);
#endif
    SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    ::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    int alen = sizeof(addr);
    ::getsockname(s, reinterpret_cast<sockaddr*>(&addr), &alen);
    port_ = ntohs(addr.sin_port);
    ::listen(s, 1);
    listen_sock_ = static_cast<intptr_t>(s);
    thread_ = std::thread([this] { run(); });
}

MockGdbServer::~MockGdbServer() {
    if (thread_.joinable())
        thread_.join();
    if (listen_sock_ >= 0)
        ::closesocket(static_cast<SOCKET>(listen_sock_));
#ifdef _WIN32
    WSACleanup();
#endif
}

static void reply(SOCKET c, const std::string& payload) {
    std::string f = frame(payload);
    ::send(c, f.c_str(), static_cast<int>(f.size()), 0);
}

void MockGdbServer::run() {
    SOCKET c = ::accept(static_cast<SOCKET>(listen_sock_), nullptr, nullptr);
    if (c == INVALID_SOCKET)
        return;
    FrameDecoder dec;
    char buf[512];
    bool done = false;
    while (!done) {
        int n = ::recv(c, buf, sizeof(buf), 0);
        if (n <= 0)
            break;
        dec.feed(std::string_view(buf, static_cast<size_t>(n)));
        std::optional<std::string> pkt;
        while ((pkt = dec.next_payload()).has_value()) {
            // Ack the received packet.
            ::send(c, "+", 1, 0);
            const std::string& p = *pkt;
            if (p == "qSupported") {
                reply(c, "PacketSize=1000");
            } else if (p == "g") {
                reply(c, "0102030405060708"); // 8 bytes of fake register block
            } else if (!p.empty() && p[0] == 'm') {
                reply(c, "deadbeef"); // 4 bytes
            } else if (!p.empty() && (p[0] == 'M' || p[0] == 'Z' || p[0] == 'z')) {
                reply(c, "OK");
            } else if (p == "c" || p == "s" || p == "?") {
                reply(c, "S05");
            } else {
                reply(c, ""); // unsupported
            }
        }
    }
    ::closesocket(c);
}
