// SPDX-License-Identifier: BUSL-1.1
#include "irida/proto/frame.hpp"
#include <cassert>
#include <string>

using namespace irida::proto;

int main() {
    // checksum: sum of bytes mod 256, hex. "OK" = 0x4F+0x4B = 0x9A
    assert(checksum("OK") == 0x9a);
    assert(frame("OK") == "$OK#9a");
    assert(frame("g") == "$g#67"); // 'g' = 0x67

    // Decoder: single packet, with a leading ack to strip.
    FrameDecoder d;
    d.feed("+$OK#9a");
    auto p = d.next_payload();
    assert(p.has_value() && *p == "OK");
    assert(!d.next_payload().has_value());

    // Split across feeds.
    FrameDecoder d2;
    d2.feed("$he");
    assert(!d2.next_payload().has_value());
    d2.feed("llo#");        // checksum of "hello" = 0x68+0x65+0x6c+0x6c+0x6f=0x214 ->0x14
    d2.feed("14");
    auto q = d2.next_payload();
    assert(q.has_value() && *q == "hello");

    // Two packets in one feed.
    FrameDecoder d3;
    d3.feed("$a#61$b#62");
    auto a = d3.next_payload(); auto b = d3.next_payload();
    assert(a && *a == "a"); assert(b && *b == "b");

    // Bad checksum -> no payload returned.
    FrameDecoder d4;
    d4.feed("$OK#00");
    assert(!d4.next_payload().has_value());
    return 0;
}
