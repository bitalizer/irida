// SPDX-License-Identifier: BUSL-1.1
#include "irida/transport/gdb_client.hpp"
#include "irida/proto/hex.hpp"

namespace irida::transport {

using irida::base::Result;
namespace proto = irida::proto;

Result<std::monostate> GdbClient::connect(std::string_view host, uint16_t port) {
    auto r = sock_.connect(host, port);
    if (!r.has_value())
        return Result<std::monostate>::err(r.error());
    // Handshake: announce features. We don't strictly require the reply.
    (void)transact(proto::req_qsupported());
    return Result<std::monostate>::ok(std::monostate{});
}

Result<std::string> GdbClient::transact(std::string_view payload) {
    using R = Result<std::string>;
    std::string framed = proto::frame(payload);
    auto sr = sock_.send({reinterpret_cast<const std::byte*>(framed.data()), framed.size()});
    if (!sr.has_value())
        return R::err(sr.error());

    // Read until the decoder yields a payload (acks are consumed internally).
    std::byte buf[512];
    for (int attempts = 0; attempts < 64; ++attempts) {
        if (auto p = decoder_.next_payload(); p.has_value())
            return R::ok(*p);
        auto rr = sock_.recv({buf, sizeof(buf)}, kTimeoutMs);
        if (!rr.has_value())
            return R::err(rr.error());
        decoder_.feed(std::string_view(reinterpret_cast<const char*>(buf), rr.value()));
    }
    return R::err("transact: no reply within attempt budget");
}

Result<std::vector<std::byte>> GdbClient::read_registers() {
    using R = Result<std::vector<std::byte>>;
    auto t = transact(proto::req_read_registers());
    if (!t.has_value())
        return R::err(t.error());
    auto d = proto::hex_decode(t.value());
    if (!d.has_value())
        return R::err(d.error());
    return R::ok(d.value());
}

Result<std::vector<std::byte>> GdbClient::read_memory(uint64_t addr, uint64_t len) {
    using R = Result<std::vector<std::byte>>;
    auto t = transact(proto::req_read_memory(addr, len));
    if (!t.has_value())
        return R::err(t.error());
    if (auto e = proto::reply_error_code(t.value()); e.has_value())
        return R::err("read_memory: target error E" + std::to_string(*e));
    auto d = proto::hex_decode(t.value());
    if (!d.has_value())
        return R::err(d.error());
    return R::ok(d.value());
}

Result<std::monostate> GdbClient::write_memory(uint64_t addr, std::span<const std::byte> data) {
    using R = Result<std::monostate>;
    auto t = transact(proto::req_write_memory(addr, data));
    if (!t.has_value())
        return R::err(t.error());
    if (!proto::reply_is_ok(t.value()))
        return R::err("write_memory: not OK (" + t.value() + ")");
    return R::ok(std::monostate{});
}

static Result<std::monostate> expect_ok(Result<std::string> t, const char* what) {
    using R = Result<std::monostate>;
    if (!t.has_value())
        return R::err(t.error());
    if (!irida::proto::reply_is_ok(t.value()))
        return R::err(std::string(what) + ": not OK (" + t.value() + ")");
    return R::ok(std::monostate{});
}

Result<std::monostate> GdbClient::set_breakpoint(int type, uint64_t addr, int kind) {
    return expect_ok(transact(proto::req_set_breakpoint(type, addr, kind)), "set_breakpoint");
}

Result<std::monostate> GdbClient::remove_breakpoint(int type, uint64_t addr, int kind) {
    return expect_ok(transact(proto::req_remove_breakpoint(type, addr, kind)), "remove_breakpoint");
}

static Result<proto::StopReply> as_stop(Result<std::string> t) {
    using R = Result<proto::StopReply>;
    if (!t.has_value())
        return R::err(t.error());
    return R::ok(proto::parse_stop_reply(t.value()));
}

Result<proto::StopReply> GdbClient::cont() {
    return as_stop(transact(proto::req_continue()));
}
Result<proto::StopReply> GdbClient::step() {
    return as_stop(transact(proto::req_step()));
}
Result<proto::StopReply> GdbClient::halt_reason() {
    return as_stop(transact(proto::req_halt_reason()));
}

void GdbClient::disconnect() {
    sock_.close();
}

} // namespace irida::transport
