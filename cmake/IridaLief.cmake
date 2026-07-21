# SPDX-License-Identifier: BUSL-1.1
# Fetches LIEF (Apache-2.0-licensed multi-format binary parsing library) via
# CMake FetchContent. Exports the CMake target LIEF::LIEF (static lib,
# BUILD_SHARED_LIBS=OFF), with its Python/C APIs, examples, tests and docs
# disabled to keep the build minimal. Only irida_binfmt's LIEF backend links
# this target.
include(FetchContent)

set(LIEF_PYTHON_API OFF CACHE BOOL "" FORCE)
set(LIEF_C_API OFF CACHE BOOL "" FORCE)
set(LIEF_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIEF_TESTS OFF CACHE BOOL "" FORCE)
set(LIEF_DOC OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  lief
  GIT_REPOSITORY https://github.com/lief-project/LIEF.git
  GIT_TAG 0.15.1)
FetchContent_MakeAvailable(lief)

# LIEF's own CMakeLists.txt only creates the namespaced "LIEF::LIEF" target
# via its install(EXPORT ...) path (for find_package consumers). For an
# in-tree FetchContent build the raw target is just "LIEF", so add the
# alias ourselves to give callers a stable, namespaced name.
if(TARGET LIEF AND NOT TARGET LIEF::LIEF)
  add_library(LIEF::LIEF ALIAS LIEF)
endif()
