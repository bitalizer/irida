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

add_subdirectory(libs/base)
add_subdirectory(libs/protocol)
add_subdirectory(capi)
add_subdirectory(frontends/mock)
if(IRIDA_GUI)
  add_subdirectory(frontends/gui)
endif()
# (remaining subdirs added as their tasks land; see plan)

# Enforce the allowed dependency DAG at configure time.
include(IridaLayerCheck)
irida_run_layer_check()
