# SPDX-License-Identifier: BUSL-1.1
# irida_backend — the abstract debug-backend seam. GdbBackend (src/gdb_backend.cpp)
# is the gdb-remote transport; NativeBackend (src/native_backend.cpp, WIN32-only)
# is the Win32 debug-API transport, over irida_host's NativeDebuggee facade.
add_library(irida_backend STATIC src/backend.cpp src/gdb_backend.cpp src/native_backend_factory.cpp)
target_include_directories(irida_backend
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(irida_backend PUBLIC irida_base PRIVATE irida_transport irida_proto)
if(WIN32)
  target_sources(irida_backend PRIVATE src/native_backend.cpp)
  target_link_libraries(irida_backend PRIVATE irida_host)
endif()
irida_set_warnings(irida_backend)

add_subdirectory(tests)
