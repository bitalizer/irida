# SPDX-License-Identifier: BUSL-1.1
add_library(irida_capi STATIC src/capi.cpp src/capi_target.cpp)
target_include_directories(irida_capi
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(irida_capi PRIVATE irida_base irida_target irida_disasm irida_backend)
if(WIN32)
  # native_backend.hpp (a public irida_backend header) pulls in irida/host's
  # NativeDebuggee facade, so any consumer including it needs irida_host too.
  target_link_libraries(irida_capi PRIVATE irida_host)
endif()
irida_set_warnings(irida_capi)

add_subdirectory(tests)
