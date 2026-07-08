// SPDX-License-Identifier: BUSL-1.1
#include "scripted_gdb_server.hpp"
#include "irida/proto/frame.hpp"
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

using irida::proto::frame;
using irida::proto::FrameDecoder;

namespace {

// Builds a hex string of `len` bytes, all equal to `fill`.
std::string hex_page(size_t len, unsigned char fill) {
    static const char* kDigits = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    char hi = kDigits[(fill >> 4) & 0xF];
    char lo = kDigits[fill & 0xF];
    for (size_t i = 0; i < len; ++i) {
        out.push_back(hi);
        out.push_back(lo);
    }
    return out;
}

} // namespace

ScriptedGdbServer::ScriptedGdbServer() {
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

ScriptedGdbServer::~ScriptedGdbServer() {
    if (thread_.joinable())
        thread_.join();
    if (listen_sock_ >= 0)
        ::closesocket(static_cast<SOCKET>(listen_sock_));
#ifdef _WIN32
    WSACleanup();
#endif
}

void ScriptedGdbServer::flip() {
    flipped_.store(true);
}

static void reply(SOCKET c, const std::string& payload) {
    std::string f = frame(payload);
    ::send(c, f.c_str(), static_cast<int>(f.size()), 0);
}

void ScriptedGdbServer::run() {
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
            ::send(c, "+", 1, 0); // ack
            const std::string& p = *pkt;
            if (p == "qSupported") {
                reply(c, "PacketSize=1000");
            } else if (p == "g") {
                reply(c, "0102030405060708");
            } else if (!p.empty() && p[0] == 'm') {
                // Parse "m<addr>,<len>" to know how many bytes to fill.
                size_t comma = p.find(',');
                uint64_t len = 4;
                if (comma != std::string::npos) {
                    len = std::stoull(p.substr(comma + 1), nullptr, 16);
                }
                unsigned char fill = flipped_.load() ? 0xBB : 0xAA;
                reply(c, hex_page(static_cast<size_t>(len), fill));
            } else if (!p.empty() && (p[0] == 'M' || p[0] == 'Z' || p[0] == 'z')) {
                reply(c, "OK");
            } else if (p == "c" || p == "s" || p == "?") {
                reply(c, "S05");
            } else {
                reply(c, "");
            }
        }
    }
    ::closesocket(c);
}
