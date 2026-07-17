// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// The abstract Backend interface is header-only (backend.hpp). This translation
// unit exists so `irida_backend` is a normal static library with a stable set of
// sources across the Phase-B merges that add gdb_backend.cpp / native_backend.cpp.
#include "irida/backend/backend.hpp"

namespace irida::backend {

// Intentionally empty: interface has no out-of-line members yet.

} // namespace irida::backend
