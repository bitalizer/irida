// SPDX-License-Identifier: BUSL-1.1
#include "irida/binfmt/binary.hpp"
#include <cassert>

#if defined(_WIN32)

using irida::binfmt::BinaryInfo;

int main() {
    auto parser = irida::binfmt::make_lief_parser();
    assert(parser != nullptr);

    auto result = parser->parse_file("C:\\Windows\\System32\\kernel32.dll");
    assert(result.has_value());

    const BinaryInfo& info = result.value();
    assert(info.format == "PE");
    assert(!info.sections.empty());
    assert(!info.exports.empty());
    assert(info.entry_point != 0 || info.image_base != 0);

    // Nonexistent file -> parse failure, not a thrown exception.
    auto missing = parser->parse_file("C:\\this\\path\\does\\not\\exist.dll");
    assert(!missing.has_value());

    return 0;
}

#else

int main() {
    return 0;
}

#endif
