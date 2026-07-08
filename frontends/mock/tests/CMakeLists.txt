# SPDX-License-Identifier: BUSL-1.1
add_executable(test_mock test_mock.cpp)
target_link_libraries(test_mock PRIVATE irida_mock)
irida_set_warnings(test_mock)
add_test(NAME mock.backend COMMAND test_mock)
