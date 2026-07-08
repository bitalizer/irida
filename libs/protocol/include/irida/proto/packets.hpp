// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace irida::proto {

std::string req_read_registers();
std::string req_read_memory(uint64_t addr, uint64_t len);
std::string req_write_memory(uint64_t addr, std::span<const std::byte> data);
std::string req_set_breakpoint(int type, uint64_t addr, int kind);
std::string req_remove_breakpoint(int type, uint64_t addr, int kind);
std::string req_continue();
std::string req_step();
std::string req_halt_reason();
std::string req_qsupported();

bool reply_is_ok(std::string_view payload);
std::optional<int> reply_error_code(std::string_view payload);

struct StopReply {
    enum Kind { Signal, Exited, Terminated, Output, Unknown } kind = Unknown;
    int code = 0;
    std::vector<std::pair<std::string, std::string>> info;
    std::string output;
};

StopReply parse_stop_reply(std::string_view payload);

} // namespace irida::proto
