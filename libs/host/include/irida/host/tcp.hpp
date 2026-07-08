// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/base/result.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <variant>

namespace irida::host {

// Minimal blocking TCP client. Platform (Winsock) details live only in tcp.cpp.
class TcpClient {
public:
    TcpClient() = default;
    ~TcpClient();
    TcpClient(TcpClient&&) noexcept;
    TcpClient& operator=(TcpClient&&) noexcept;
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    irida::base::Result<std::monostate> connect(std::string_view host, uint16_t port);
    irida::base::Result<size_t> send(std::span<const std::byte> data);
    irida::base::Result<size_t> recv(std::span<std::byte> buf, int timeout_ms);
    void close();

private:
    // Stored as intptr_t to avoid leaking Winsock types into the header.
    intptr_t sock_ = -1;
};

} // namespace irida::host
