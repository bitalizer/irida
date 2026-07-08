# SPDX-License-Identifier: BUSL-1.1
# Copyright (c) 2026 Bitalizer. See LICENSE (Business Source License 1.1).
cmake_minimum_required(VERSION 3.21)
project(irida LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(IRIDA_GUI "Build the Qt 6 GUI frontend" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(IridaWarnings)

enable_testing()

# Three shippable projects: irida-core (engine), irida-api (stable C ABI),
# irida-ui (frontends). Build together for now; the layout is the seam along
# which they will later split into separately-shipped artifacts.

# --- irida-core: the engine ---
add_subdirectory(irida-core/libs/base)
add_subdirectory(irida-core/libs/protocol)
add_subdirectory(irida-core/libs/host)
add_subdirectory(irida-core/core/transport)
add_subdirectory(irida-core/core/target)

# --- irida-api: the stable C ABI ---
add_subdirectory(irida-api)

# --- irida-ui: frontends (mock backend + Qt GUI) ---
add_subdirectory(irida-ui/mock)
if(IRIDA_GUI)
  add_subdirectory(irida-ui/gui)
endif()

# Enforce the allowed dependency DAG at configure time.
include(IridaLayerCheck)
irida_run_layer_check()
