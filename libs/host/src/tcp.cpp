// SPDX-License-Identifier: BUSL-1.1
#include "irida/host/tcp.hpp"
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#error "irida::host::TcpClient currently supports Windows only"
#endif

namespace irida::host {

namespace {
struct WsaGuard {
    WsaGuard() {
        WSADATA w;
        WSAStartup(MAKEWORD(2, 2), &w);
    }
    ~WsaGuard() {
        WSACleanup();
    }
};
// Process-wide init; constructed on first TcpClient use.
void ensure_wsa() {
    static WsaGuard g;
}
} // namespace

using R0 = irida::base::Result<std::monostate>;
using Rn = irida::base::Result<size_t>;

TcpClient::~TcpClient() {
    close();
}

TcpClient::TcpClient(TcpClient&& o) noexcept : sock_(o.sock_) {
    o.sock_ = -1;
}
TcpClient& TcpClient::operator=(TcpClient&& o) noexcept {
    if (this != &o) {
        close();
        sock_ = o.sock_;
        o.sock_ = -1;
    }
    return *this;
}

R0 TcpClient::connect(std::string_view host, uint16_t port) {
    ensure_wsa();
    SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        return R0::err("socket() failed");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    std::string h(host);
    if (::inet_pton(AF_INET, h.c_str(), &addr.sin_addr) != 1) {
        ::closesocket(s);
        return R0::err("invalid host address: " + h);
    }
    if (::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::closesocket(s);
        return R0::err("connect() failed");
    }
    sock_ = static_cast<intptr_t>(s);
    return R0::ok(std::monostate{});
}

Rn TcpClient::send(std::span<const std::byte> data) {
    if (sock_ < 0)
        return Rn::err("send: not connected");
    int n = ::send(static_cast<SOCKET>(sock_), reinterpret_cast<const char*>(data.data()),
                   static_cast<int>(data.size()), 0);
    if (n == SOCKET_ERROR)
        return Rn::err("send() failed");
    return Rn::ok(static_cast<size_t>(n));
}

Rn TcpClient::recv(std::span<std::byte> buf, int timeout_ms) {
    if (sock_ < 0)
        return Rn::err("recv: not connected");
    DWORD tv = static_cast<DWORD>(timeout_ms);
    ::setsockopt(static_cast<SOCKET>(sock_), SOL_SOCKET, SO_RCVTIMEO,
                 reinterpret_cast<const char*>(&tv), sizeof(tv));
    int n = ::recv(static_cast<SOCKET>(sock_), reinterpret_cast<char*>(buf.data()),
                   static_cast<int>(buf.size()), 0);
    if (n == SOCKET_ERROR)
        return Rn::err("recv() failed or timed out");
    if (n == 0)
        return Rn::err("recv: connection closed");
    return Rn::ok(static_cast<size_t>(n));
}

void TcpClient::close() {
    if (sock_ >= 0) {
        ::closesocket(static_cast<SOCKET>(sock_));
        sock_ = -1;
    }
}

} // namespace irida::host
