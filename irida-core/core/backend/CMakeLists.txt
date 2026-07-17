# SPDX-License-Identifier: BUSL-1.1
# irida_backend — the abstract debug-backend seam. GdbBackend (src/gdb_backend.cpp)
# and NativeBackend (src/native_backend.cpp, WIN32) are added by later phases.
add_library(irida_backend STATIC src/backend.cpp src/gdb_backend.cpp)
target_include_directories(irida_backend
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(irida_backend PUBLIC irida_base PRIVATE irida_transport irida_proto)
irida_set_warnings(irida_backend)

add_subdirectory(tests)
