# SPDX-License-Identifier: BUSL-1.1
add_executable(test_binfmt test_binfmt.cpp)
target_link_libraries(test_binfmt PRIVATE irida_binfmt)
irida_set_warnings(test_binfmt)
add_test(NAME binfmt.binfmt COMMAND test_binfmt)
