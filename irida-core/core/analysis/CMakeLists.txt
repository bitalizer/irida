# SPDX-License-Identifier: BUSL-1.1
add_library(irida_analysis STATIC src/analysis.cpp)
target_include_directories(irida_analysis
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(irida_analysis PUBLIC irida_base irida_disasm)
irida_set_warnings(irida_analysis)

add_subdirectory(tests)
