// SPDX-License-Identifier: BUSL-1.1
#include "irida/proto/hex.hpp"
#include <cassert>
#include <cstring>

using namespace irida::proto;

int main() {
    std::byte data[] = {std::byte{0x00}, std::byte{0xde}, std::byte{0xad}, std::byte{0xff}};
    assert(hex_encode({data, 4}) == "00deadff");

    auto r = hex_decode("00deadff");
    assert(r.has_value());
    const auto& v = r.value();
    assert(v.size() == 4);
    assert(v[1] == std::byte{0xde} && v[3] == std::byte{0xff});

    assert(!hex_decode("abc").has_value()); // odd length
    assert(!hex_decode("zz").has_value());  // invalid digit

    // round-trip empty
    assert(hex_encode({}) == "");
    assert(hex_decode("").has_value() && hex_decode("").value().empty());
    return 0;
}
