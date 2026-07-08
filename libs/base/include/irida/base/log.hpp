// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <string_view>
namespace irida::base {
void log_info(std::string_view msg);
void log_error(std::string_view msg);
} // namespace irida::base
