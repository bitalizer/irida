// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// The GdbBackend attach overload is isolated in its own translation unit so that
// only executables that call attach(host, port) link against make_gdb_backend().
#include "irida/target/target.hpp"
#include <utility>

namespace irida::backend {
std::unique_ptr<Backend> make_gdb_backend();
} // namespace irida::backend

namespace irida::target {

using irida::base::Result;
namespace backend = irida::backend;

Result<Target> Target::attach(std::string_view host, uint16_t port) {
    auto b = backend::make_gdb_backend();

    backend::AttachSpec spec;
    spec.method = backend::AttachMethod::GdbRemote;
    spec.host = std::string(host);
    spec.port = port;

    auto conn = b->attach(spec);
    if (!conn.has_value())
        return Result<Target>::err(conn.error());

    return Target::attach(std::move(b));
}

} // namespace irida::target
