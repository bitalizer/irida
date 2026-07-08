// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/irida.h"

struct IridaSession {
    IridaBackendVTable vt;
    void* ctx;
};
