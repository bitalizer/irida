// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
//
// The ONLY file in this project that includes LIEF headers. Everything
// above this file (the BinaryParser interface, Section/Import/Export/
// Symbol/BinaryInfo types) is LIEF-free by design so the parsing backend
// is swappable later.
#include "irida/binfmt/binary.hpp"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include <LIEF/PE.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

namespace irida::binfmt {

namespace {

uint8_t map_perms(const LIEF::PE::Section& section) {
    uint8_t perms = 0;
    if (section.has_characteristic(LIEF::PE::Section::CHARACTERISTICS::MEM_READ)) {
        perms |= 0b100;
    }
    if (section.has_characteristic(LIEF::PE::Section::CHARACTERISTICS::MEM_WRITE)) {
        perms |= 0b010;
    }
    if (section.has_characteristic(LIEF::PE::Section::CHARACTERISTICS::MEM_EXECUTE)) {
        perms |= 0b001;
    }
    return perms;
}

Section map_section(const LIEF::PE::Section& section) {
    Section out;
    out.name = section.name();
    out.vaddr = section.virtual_address();
    out.vsize = section.virtual_size();
    out.offset = section.pointerto_raw_data();
    out.size = section.sizeof_raw_data();
    out.perms = map_perms(section);
    return out;
}

void collect_imports(const LIEF::PE::Binary& binary, std::vector<Import>& out) {
    for (const LIEF::PE::Import& import : binary.imports()) {
        for (const LIEF::PE::ImportEntry& entry : import.entries()) {
            Import mapped;
            mapped.name = entry.name();
            mapped.library = import.name();
            mapped.iat_addr = entry.iat_address();
            out.push_back(std::move(mapped));
        }
    }
}

void collect_exports(const LIEF::PE::Binary& binary, std::vector<Export>& out) {
    if (!binary.has_exports()) {
        return;
    }
    const LIEF::PE::Export* exp = binary.get_export();
    if (exp == nullptr) {
        return;
    }
    for (const LIEF::PE::ExportEntry& entry : exp->entries()) {
        Export mapped;
        mapped.name = entry.name();
        mapped.addr = entry.address();
        mapped.ordinal = entry.ordinal();
        out.push_back(std::move(mapped));
    }
}

void collect_symbols(const LIEF::PE::Binary& binary, std::vector<Symbol>& out) {
    for (const LIEF::PE::Symbol& symbol : binary.symbols()) {
        Symbol mapped;
        mapped.name = symbol.name();
        mapped.addr = symbol.value();
        mapped.kind = "coff";
        out.push_back(std::move(mapped));
    }
}

class LiefParser final : public BinaryParser {
  public:
    irida::base::Result<BinaryInfo> parse_file(const std::string& path) override {
        try {
            std::unique_ptr<LIEF::PE::Binary> binary = LIEF::PE::Parser::parse(path);
            if (binary == nullptr) {
                return irida::base::Result<BinaryInfo>::err("failed to parse PE file: " + path);
            }

            BinaryInfo info;
            info.path = path;
            info.format = "PE";
            info.entry_point = binary->entrypoint();
            info.image_base = binary->imagebase();
            info.is_64bit = binary->type() == LIEF::PE::PE_TYPE::PE32_PLUS;

            info.sections.reserve(binary->sections().size());
            for (const LIEF::PE::Section& section : binary->sections()) {
                info.sections.push_back(map_section(section));
            }

            collect_imports(*binary, info.imports);
            collect_exports(*binary, info.exports);
            collect_symbols(*binary, info.symbols);

            // LIEF reports section/import/export/symbol addresses as RVAs but
            // the entry point as an absolute VA. Rebase the RVAs by the image
            // base so every address in BinaryInfo lives in one space (absolute
            // VA), matching the entry point and how a loaded module is seen.
            const uint64_t base = info.image_base;
            for (auto& sec : info.sections)
                sec.vaddr += base;
            for (auto& imp : info.imports)
                imp.iat_addr += base;
            for (auto& exp : info.exports)
                exp.addr += base;
            for (auto& sym : info.symbols)
                sym.addr += base;

            return irida::base::Result<BinaryInfo>::ok(std::move(info));
        } catch (const std::exception& e) {
            return irida::base::Result<BinaryInfo>::err(std::string("LIEF exception: ") + e.what());
        } catch (...) {
            return irida::base::Result<BinaryInfo>::err("LIEF exception: unknown error parsing " +
                                                        path);
        }
    }
};

} // namespace

std::unique_ptr<BinaryParser> make_lief_parser() {
    return std::make_unique<LiefParser>();
}

} // namespace irida::binfmt
