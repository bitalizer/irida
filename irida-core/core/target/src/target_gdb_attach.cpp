// SPDX-License-Identifier: BUSL-1.1
//
// Target::attach(host, port) — the GdbBackend convenience overload — lives in its own
// translation unit (separate from target.cpp) so that linking it is opt-in: it is the
// ONLY place on this branch that references irida::backend::make_gdb_backend(), which is
// implemented in a separate lane (core/backend/src/gdb_backend.cpp) not present here.
// Executables that don't call attach(host, port) never pull this .obj's unresolved
// reference into their link.
// TODO(merge): make_gdb_backend() provided by gdb-impl lane.
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
