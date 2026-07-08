# SPDX-License-Identifier: BUSL-1.1
add_executable(test_base_result test_result.cpp)
target_link_libraries(test_base_result PRIVATE irida_base)
irida_set_warnings(test_base_result)
add_test(NAME base.result COMMAND test_base_result)
