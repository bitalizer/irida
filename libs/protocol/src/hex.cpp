// SPDX-License-Identifier: BUSL-1.1
#include "irida/proto/hex.hpp"

namespace irida::proto {

std::string hex_encode(std::span<const std::byte> bytes) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (std::byte b : bytes) {
        auto v = static_cast<unsigned>(b);
        out.push_back(digits[(v >> 4) & 0xf]);
        out.push_back(digits[v & 0xf]);
    }
    return out;
}

static int hexval(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

irida::base::Result<std::vector<std::byte>> hex_decode(std::string_view hex) {
    using R = irida::base::Result<std::vector<std::byte>>;
    if (hex.size() % 2 != 0)
        return R::err("hex_decode: odd length");
    std::vector<std::byte> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = hexval(hex[i]);
        int lo = hexval(hex[i + 1]);
        if (hi < 0 || lo < 0)
            return R::err("hex_decode: invalid digit");
        out.push_back(static_cast<std::byte>((hi << 4) | lo));
    }
    return R::ok(std::move(out));
}

} // namespace irida::proto
