# SPDX-License-Identifier: BUSL-1.1
# Fetches Zydis (MIT-licensed x86/x86-64 disassembler library) via
# CMake FetchContent. Exports the CMake target Zydis::Zydis (static lib,
# ZYDIS_BUILD_SHARED_LIB=OFF), pulling its own Zycore dependency
# automatically. Only irida_disasm's Zydis backend links this target.
include(FetchContent)

set(ZYDIS_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  zydis
  GIT_REPOSITORY https://github.com/zyantific/zydis.git
  GIT_TAG v4.1.0)
FetchContent_MakeAvailable(zydis)

# Zydis's own CMakeLists.txt only creates the namespaced "Zydis::Zydis"
# target via its install(EXPORT ...) path (for find_package consumers).
# For an in-tree FetchContent build the raw target is just "Zydis", so
# add the alias ourselves to give callers a stable, namespaced name.
if(TARGET Zydis AND NOT TARGET Zydis::Zydis)
  add_library(Zydis::Zydis ALIAS Zydis)
endif()
