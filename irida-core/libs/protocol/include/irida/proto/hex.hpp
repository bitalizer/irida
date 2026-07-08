// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/base/result.hpp"
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace irida::proto {

std::string hex_encode(std::span<const std::byte> bytes);
irida::base::Result<std::vector<std::byte>> hex_decode(std::string_view hex);

} // namespace irida::proto
