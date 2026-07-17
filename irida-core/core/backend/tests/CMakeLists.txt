# SPDX-License-Identifier: BUSL-1.1
add_executable(test_mock_backend test_mock_backend.cpp)
target_include_directories(test_mock_backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_mock_backend PRIVATE irida_backend)
irida_set_warnings(test_mock_backend)
add_test(NAME backend.mock COMMAND test_mock_backend)
