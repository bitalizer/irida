# SPDX-License-Identifier: BUSL-1.1
include(IridaLief)

add_library(irida_binfmt STATIC src/lief_parser.cpp)
target_include_directories(irida_binfmt
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(irida_binfmt PUBLIC irida_base PRIVATE LIEF::LIEF)
irida_set_warnings(irida_binfmt)

add_subdirectory(tests)
