// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/base/bytes.hpp"
#include "irida/base/result.hpp"
#include "irida/host/tcp.hpp"
#include "irida/proto/frame.hpp"
#include "irida/proto/packets.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace irida::transport {

class GdbClient {
  public:
    irida::base::Result<std::monostate> connect(std::string_view host, uint16_t port);
    irida::base::Result<irida::base::Bytes> read_registers();
    irida::base::Result<irida::base::Bytes> read_memory(uint64_t addr, uint64_t len);
    irida::base::Result<std::monostate> write_memory(uint64_t addr,
                                                     std::span<const std::byte> data);
    irida::base::Result<std::monostate> set_breakpoint(int type, uint64_t addr, int kind);
    irida::base::Result<std::monostate> remove_breakpoint(int type, uint64_t addr, int kind);
    irida::base::Result<irida::proto::StopReply> cont();
    irida::base::Result<irida::proto::StopReply> step();
    irida::base::Result<irida::proto::StopReply> halt_reason();
    void disconnect();

  private:
    // Send a payload framed, consume the ack, and return the next reply payload.
    irida::base::Result<std::string> transact(std::string_view payload);

    irida::host::TcpClient sock_;
    irida::proto::FrameDecoder decoder_;
    static constexpr int kTimeoutMs = 3000;
};

} // namespace irida::transport
