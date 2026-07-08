// SPDX-License-Identifier: BUSL-1.1
#include "irida/base/log.hpp"
#include <iostream>
namespace irida::base {
void log_info(std::string_view msg) {
    std::cerr << "[irida][info] " << msg << '\n';
}
void log_error(std::string_view msg) {
    std::cerr << "[irida][error] " << msg << '\n';
}
} // namespace irida::base
