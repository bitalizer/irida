// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace irida::binfmt {

struct FoundString {
    uint64_t addr;
    std::string text;
    bool utf16;
};

// Scans data for printable ASCII runs and UTF-16LE runs of at least min_len
// characters. addr on each result is base_addr + offset into data.
std::vector<FoundString> scan_strings(std::span<const std::byte> data, uint64_t base_addr,
                                      size_t min_len = 4);

} // namespace irida::binfmt
