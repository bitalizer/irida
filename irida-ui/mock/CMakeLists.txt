# SPDX-License-Identifier: BUSL-1.1
add_library(irida_mock STATIC src/mock_backend.cpp)
target_include_directories(irida_mock
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(irida_mock PUBLIC irida_capi)
irida_set_warnings(irida_mock)

add_subdirectory(tests)
