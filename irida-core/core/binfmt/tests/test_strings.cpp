// SPDX-License-Identifier: BUSL-1.1
#include "irida/binfmt/strings.hpp"
#include <cassert>
#include <cstring>

using irida::binfmt::FoundString;
using irida::binfmt::scan_strings;

int main() {
    std::vector<std::byte> buf(64, std::byte{0});
    const char* planted = "hello";
    std::memcpy(buf.data() + 10, planted, std::strlen(planted));

    auto found = scan_strings(buf, 0x1000, 4);

    bool saw_hello = false;
    for (const auto& s : found) {
        if (!s.utf16 && s.text == "hello") {
            saw_hello = true;
            assert(s.addr == 0x1000 + 10);
        }
    }
    assert(saw_hello);

    // UTF-16LE plant: "hi" -> h.i.
    std::vector<std::byte> wbuf(32, std::byte{0});
    const char16_t wtext[] = u"hiya";
    for (size_t i = 0; i < 4; ++i) {
        wbuf[2 + i * 2] = static_cast<std::byte>(wtext[i] & 0xFF);
        wbuf[2 + i * 2 + 1] = std::byte{0};
    }
    auto wfound = scan_strings(wbuf, 0x2000, 4);
    bool saw_hiya = false;
    for (const auto& s : wfound) {
        if (s.utf16 && s.text == "hiya") {
            saw_hiya = true;
            assert(s.addr == 0x2000 + 2);
        }
    }
    assert(saw_hiya);

    // Below min_len should not appear.
    std::vector<std::byte> shortbuf(16, std::byte{0});
    std::memcpy(shortbuf.data() + 3, "ab", 2);
    auto shortfound = scan_strings(shortbuf, 0, 4);
    for (const auto& s : shortfound)
        assert(s.text != "ab");

    return 0;
}
