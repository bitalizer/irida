# SPDX-License-Identifier: BUSL-1.1
# Copyright (c) 2026 Bitalizer. See LICENSE (Business Source License 1.1).
cmake_minimum_required(VERSION 3.21)
project(irida LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(IRIDA_GUI "Build the Qt 6 GUI frontend" ON)
option(IRIDA_TIDY "Run clang-tidy on irida-core/irida-api sources during the build" OFF)

# Always emit compile_commands.json so clang-tidy (and editors) can analyze the
# tree out of band, whether or not the inline IRIDA_TIDY pass is enabled.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(IridaWarnings)

# When IRIDA_TIDY is on, point CMAKE_CXX_CLANG_TIDY at the tool so every
# compiled core/api translation unit is linted in the same step. Third-party
# code (Zydis, LIEF) is never our source, so it is excluded via each library's
# own target properties, not here.
if(IRIDA_TIDY)
  find_program(IRIDA_CLANG_TIDY_EXE NAMES clang-tidy)
  if(IRIDA_CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${IRIDA_CLANG_TIDY_EXE}")
    message(STATUS "clang-tidy: ${IRIDA_CLANG_TIDY_EXE}")
  else()
    message(WARNING "IRIDA_TIDY=ON but clang-tidy was not found on PATH")
  endif()
endif()

enable_testing()

# Three shippable projects: irida-core (engine), irida-api (stable C ABI),
# irida-ui (frontends). Build together for now; the layout is the seam along
# which they will later split into separately-shipped artifacts.

# --- irida-core: the engine ---
add_subdirectory(irida-core/libs/base)
add_subdirectory(irida-core/libs/protocol)
add_subdirectory(irida-core/libs/host)
add_subdirectory(irida-core/core/transport)
add_subdirectory(irida-core/core/backend)
add_subdirectory(irida-core/core/target)
add_subdirectory(irida-core/core/disasm)
add_subdirectory(irida-core/core/analysis)
add_subdirectory(irida-core/core/binfmt)

# --- irida-api: the stable C ABI ---
add_subdirectory(irida-api)

# --- irida-ui: frontends (mock backend + Qt GUI + headless CLI) ---
add_subdirectory(irida-ui/mock)
add_subdirectory(irida-ui/cli)
if(IRIDA_GUI)
  add_subdirectory(irida-ui/gui)
endif()

# Enforce the allowed dependency DAG at configure time.
include(IridaLayerCheck)
irida_run_layer_check()
