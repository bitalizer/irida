# SPDX-License-Identifier: BUSL-1.1
add_library(irida_host STATIC src/tcp.cpp)
target_include_directories(irida_host
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(irida_host PUBLIC irida_base)
if(WIN32)
  target_link_libraries(irida_host PRIVATE ws2_32)
endif()
irida_set_warnings(irida_host)

add_subdirectory(tests)
