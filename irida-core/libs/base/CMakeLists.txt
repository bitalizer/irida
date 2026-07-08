# SPDX-License-Identifier: BUSL-1.1
add_library(irida_base STATIC src/log.cpp)
target_include_directories(irida_base
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
irida_set_warnings(irida_base)

add_subdirectory(tests)
