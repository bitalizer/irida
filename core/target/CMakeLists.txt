# SPDX-License-Identifier: BUSL-1.1
add_library(irida_target STATIC
  src/register_set.cpp
  src/memory_cache.cpp)
target_include_directories(irida_target
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(irida_target PUBLIC irida_transport irida_base)
irida_set_warnings(irida_target)

add_subdirectory(tests)
