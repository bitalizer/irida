// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#include "irida/binfmt/strings.hpp"

namespace irida::binfmt {

namespace {

bool is_printable(uint8_t c) {
    return c >= 0x20 && c <= 0x7E;
}

} // namespace

std::vector<FoundString> scan_strings(std::span<const std::byte> data, uint64_t base_addr,
                                      size_t min_len) {
    std::vector<FoundString> out;
    const size_t n = data.size();

    // ASCII pass: runs of printable bytes.
    {
        size_t i = 0;
        while (i < n) {
            size_t start = i;
            while (i < n && is_printable(static_cast<uint8_t>(data[i])))
                ++i;
            size_t len = i - start;
            if (len >= min_len)
                out.push_back(FoundString{
                    base_addr + start,
                    std::string(reinterpret_cast<const char*>(data.data() + start), len), false});
            ++i;
        }
    }

    // UTF-16LE pass: runs of printable-ascii-char followed by 0x00.
    {
        size_t i = 0;
        while (i + 1 < n) {
            size_t start = i;
            std::string text;
            size_t j = i;
            while (j + 1 < n) {
                auto lo = static_cast<uint8_t>(data[j]);
                auto hi = static_cast<uint8_t>(data[j + 1]);
                if (hi != 0x00 || !is_printable(lo))
                    break;
                text.push_back(static_cast<char>(lo));
                j += 2;
            }
            if (text.size() >= min_len)
                out.push_back(FoundString{base_addr + start, std::move(text), true});
            i = (j > i) ? j : i + 1;
        }
    }

    return out;
}

} // namespace irida::binfmt
