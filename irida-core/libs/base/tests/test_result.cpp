// SPDX-License-Identifier: BUSL-1.1
#include "irida/base/result.hpp"
#include <cassert>
#include <string>

using irida::base::Result;

int main() {
    Result<int> ok = Result<int>::ok(42);
    assert(ok.has_value());
    assert(ok.value() == 42);

    Result<int> bad = Result<int>::err("boom");
    assert(!bad.has_value());
    assert(bad.error() == "boom");
    return 0;
}
