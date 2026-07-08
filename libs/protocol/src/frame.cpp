// SPDX-License-Identifier: BUSL-1.1
#include "irida/proto/frame.hpp"
#include <cstdio>

namespace irida::proto {

uint8_t checksum(std::string_view payload) {
    unsigned sum = 0;
    for (char c : payload)
        sum += static_cast<unsigned char>(c);
    return static_cast<uint8_t>(sum & 0xff);
}

std::string frame(std::string_view payload) {
    char cs[3];
    std::snprintf(cs, sizeof(cs), "%02x", checksum(payload));
    std::string out;
    out.reserve(payload.size() + 4);
    out.push_back('$');
    out.append(payload);
    out.push_back('#');
    out.append(cs, 2);
    return out;
}

bool is_ack(char c) {
    return c == '+' || c == '-';
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

void FrameDecoder::feed(std::string_view bytes) {
    buf_.append(bytes);
}

std::optional<std::string> FrameDecoder::next_payload() {
    // Strip any leading ack bytes.
    size_t i = 0;
    while (i < buf_.size() && is_ack(buf_[i]))
        ++i;
    if (i > 0)
        buf_.erase(0, i);

    auto start = buf_.find('$');
    if (start == std::string::npos) {
        buf_.clear();
        return std::nullopt;
    }
    auto hash = buf_.find('#', start);
    if (hash == std::string::npos || hash + 2 >= buf_.size())
        return std::nullopt; // need more bytes (checksum digits not yet present)

    std::string payload = buf_.substr(start + 1, hash - start - 1);
    int hi = hexval(buf_[hash + 1]);
    int lo = hexval(buf_[hash + 2]);
    bool good = hi >= 0 && lo >= 0 && static_cast<uint8_t>((hi << 4) | lo) == checksum(payload);
    buf_.erase(0, hash + 3);
    if (!good)
        return std::nullopt;
    return payload;
}

} // namespace irida::proto
