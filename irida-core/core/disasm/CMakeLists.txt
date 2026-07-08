# SPDX-License-Identifier: BUSL-1.1
include(IridaZydis)

add_library(irida_disasm STATIC src/zydis_disassembler.cpp)
target_include_directories(irida_disasm
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(irida_disasm PUBLIC irida_base PRIVATE Zydis::Zydis)
irida_set_warnings(irida_disasm)

add_subdirectory(tests)
