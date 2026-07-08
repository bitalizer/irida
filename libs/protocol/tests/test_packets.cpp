// SPDX-License-Identifier: BUSL-1.1
#include "irida/proto/packets.hpp"
#include <cassert>

using namespace irida::proto;

int main() {
    assert(req_read_registers() == "g");
    assert(req_read_memory(0x1000, 4) == "m1000,4");
    assert(req_read_memory(0, 0) == "m0,0");
    assert(req_continue() == "c");
    assert(req_step() == "s");
    assert(req_halt_reason() == "?");
    assert(req_set_breakpoint(0, 0x401000, 1) == "Z0,401000,1");
    assert(req_remove_breakpoint(1, 0x401000, 1) == "z1,401000,1");

    std::byte bytes[] = {std::byte{0xde}, std::byte{0xad}};
    assert(req_write_memory(0x2000, {bytes, 2}) == "M2000,2:dead");

    assert(reply_is_ok("OK"));
    assert(!reply_is_ok("E01"));
    assert(reply_error_code("E01").value() == 1);
    assert(!reply_error_code("OK").has_value());

    StopReply s = parse_stop_reply("S05");
    assert(s.kind == StopReply::Signal && s.code == 5);

    StopReply t = parse_stop_reply("T05thread:1;");
    assert(t.kind == StopReply::Signal && t.code == 5);
    assert(t.info.size() == 1 && t.info[0].first == "thread" && t.info[0].second == "1");

    StopReply w = parse_stop_reply("W00");
    assert(w.kind == StopReply::Exited && w.code == 0);

    StopReply x = parse_stop_reply("X0b");
    assert(x.kind == StopReply::Terminated && x.code == 11);
    return 0;
}
