// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include <cstddef>
#include <vector>

namespace irida::base {

// A buffer of raw bytes. Used pervasively for memory reads, register blocks,
// and wire payloads.
using Bytes = std::vector<std::byte>;

} // namespace irida::base
