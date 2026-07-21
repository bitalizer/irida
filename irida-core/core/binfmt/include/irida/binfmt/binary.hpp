// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include "irida/base/result.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace irida::binfmt {

struct Section {
    std::string name;
    uint64_t vaddr;
    uint64_t vsize;
    uint64_t offset;
    uint64_t size;
    uint8_t perms;
};

struct Import {
    std::string name;
    std::string library;
    uint64_t iat_addr;
};

struct Export {
    std::string name;
    uint64_t addr;
    uint32_t ordinal;
};

struct Symbol {
    std::string name;
    uint64_t addr;
    std::string kind;
};

struct BinaryInfo {
    std::string path;
    std::string format;
    uint64_t entry_point;
    uint64_t image_base;
    bool is_64bit;
    std::vector<Section> sections;
    std::vector<Import> imports;
    std::vector<Export> exports;
    std::vector<Symbol> symbols;
};

class BinaryParser {
  public:
    virtual ~BinaryParser() = default;
    virtual irida::base::Result<BinaryInfo> parse_file(const std::string& path) = 0;
};

std::unique_ptr<BinaryParser> make_lief_parser();

} // namespace irida::binfmt
