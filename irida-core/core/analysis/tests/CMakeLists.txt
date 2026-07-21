# SPDX-License-Identifier: BUSL-1.1
add_executable(test_analysis test_analysis.cpp)
target_link_libraries(test_analysis PRIVATE irida_analysis)
irida_set_warnings(test_analysis)
add_test(NAME analysis.analysis COMMAND test_analysis)
