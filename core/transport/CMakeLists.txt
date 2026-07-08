# SPDX-License-Identifier: BUSL-1.1
add_library(irida_transport STATIC src/gdb_client.cpp)
target_include_directories(irida_transport
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(irida_transport PUBLIC irida_proto irida_host PRIVATE irida_base)
irida_set_warnings(irida_transport)

add_subdirectory(tests)
