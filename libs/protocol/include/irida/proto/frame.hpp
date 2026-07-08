// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace irida::proto {

uint8_t checksum(std::string_view payload);
std::string frame(std::string_view payload);
bool is_ack(char c);

// Incrementally deframes a byte stream into checksum-verified payloads.
// Leading '+'/'-' ack bytes are consumed silently. Bad-checksum packets are
// dropped (return no payload for that packet).
class FrameDecoder {
public:
    void feed(std::string_view bytes);
    std::optional<std::string> next_payload();

private:
    std::string buf_;
};

} // namespace irida::proto
