# SPDX-License-Identifier: BUSL-1.1
add_executable(test_mock_backend test_mock_backend.cpp)
target_include_directories(test_mock_backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_mock_backend PRIVATE irida_backend)
irida_set_warnings(test_mock_backend)
add_test(NAME backend.mock COMMAND test_mock_backend)

if(WIN32)
  add_executable(native_helper_debuggee native_helper_debuggee.cpp)
  irida_set_warnings(native_helper_debuggee)

  add_executable(test_native_backend_win test_native_backend_win.cpp)
  target_link_libraries(test_native_backend_win PRIVATE irida_backend irida_host)
  target_compile_definitions(test_native_backend_win PRIVATE
    "IRIDA_NATIVE_HELPER_PATH=\"$<TARGET_FILE:native_helper_debuggee>\"")
  add_dependencies(test_native_backend_win native_helper_debuggee)
  irida_set_warnings(test_native_backend_win)
  add_test(NAME backend.native_win COMMAND test_native_backend_win)
endif()
