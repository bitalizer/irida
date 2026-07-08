# SPDX-License-Identifier: BUSL-1.1
add_library(irida_proto STATIC src/frame.cpp)
target_include_directories(irida_proto
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
irida_set_warnings(irida_proto)

add_subdirectory(tests)
