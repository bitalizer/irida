// SPDX-License-Identifier: BUSL-1.1
#include "irida/proto/packets.hpp"
#include "irida/proto/hex.hpp"
#include <cstdio>

namespace irida::proto {

static std::string hexnum(uint64_t v) {
    char buf[17];
    std::snprintf(buf, sizeof(buf), "%llx", static_cast<unsigned long long>(v));
    return std::string(buf);
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

static int parse_hex_byte(std::string_view s) {
    if (s.size() < 2)
        return -1;
    int hi = hexval(s[0]), lo = hexval(s[1]);
    if (hi < 0 || lo < 0)
        return -1;
    return (hi << 4) | lo;
}

std::string req_read_registers() { return "g"; }
std::string req_read_memory(uint64_t addr, uint64_t len) {
    return "m" + hexnum(addr) + "," + hexnum(len);
}
std::string req_write_memory(uint64_t addr, std::span<const std::byte> data) {
    return "M" + hexnum(addr) + "," + hexnum(data.size()) + ":" + hex_encode(data);
}
std::string req_set_breakpoint(int type, uint64_t addr, int kind) {
    return "Z" + std::to_string(type) + "," + hexnum(addr) + "," + std::to_string(kind);
}
std::string req_remove_breakpoint(int type, uint64_t addr, int kind) {
    return "z" + std::to_string(type) + "," + hexnum(addr) + "," + std::to_string(kind);
}
std::string req_continue() { return "c"; }
std::string req_step() { return "s"; }
std::string req_halt_reason() { return "?"; }
std::string req_qsupported() { return "qSupported"; }

bool reply_is_ok(std::string_view payload) { return payload == "OK"; }

std::optional<int> reply_error_code(std::string_view payload) {
    if (payload.size() >= 3 && payload[0] == 'E') {
        int b = parse_hex_byte(payload.substr(1));
        if (b >= 0)
            return b;
    }
    return std::nullopt;
}

StopReply parse_stop_reply(std::string_view p) {
    StopReply r;
    if (p.empty())
        return r;
    char t = p[0];
    if (t == 'O') {
        r.kind = StopReply::Output;
        r.output = std::string(p.substr(1));
        return r;
    }
    int code = parse_hex_byte(p.substr(1));
    r.code = code < 0 ? 0 : code;
    switch (t) {
    case 'S':
        r.kind = StopReply::Signal;
        break;
    case 'T': {
        r.kind = StopReply::Signal;
        // key:val; pairs after the 2-digit signal. Guard against a short/
        // malformed reply (e.g. "T" or "T0") whose substr(3) would throw.
        std::string_view rest = p.size() > 3 ? p.substr(3) : std::string_view{};
        size_t i = 0;
        while (i < rest.size()) {
            size_t semi = rest.find(';', i);
            std::string_view field = rest.substr(i, semi == std::string_view::npos
                                                        ? std::string_view::npos
                                                        : semi - i);
            size_t colon = field.find(':');
            if (colon != std::string_view::npos) {
                r.info.emplace_back(std::string(field.substr(0, colon)),
                                    std::string(field.substr(colon + 1)));
            }
            if (semi == std::string_view::npos)
                break;
            i = semi + 1;
        }
        break;
    }
    case 'W':
        r.kind = StopReply::Exited;
        break;
    case 'X':
        r.kind = StopReply::Terminated;
        break;
    default:
        r.kind = StopReply::Unknown;
        break;
    }
    return r;
}

} // namespace irida::proto
