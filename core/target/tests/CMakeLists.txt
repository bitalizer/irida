# SPDX-License-Identifier: BUSL-1.1
add_executable(test_register_set test_register_set.cpp)
target_link_libraries(test_register_set PRIVATE irida_target)
irida_set_warnings(test_register_set)
add_test(NAME target.register_set COMMAND test_register_set)
