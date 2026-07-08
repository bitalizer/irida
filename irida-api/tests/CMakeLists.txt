# SPDX-License-Identifier: BUSL-1.1
add_executable(test_capi_debug test_capi_debug.cpp)
target_link_libraries(test_capi_debug PRIVATE irida_capi)
irida_set_warnings(test_capi_debug)
add_test(NAME capi.debug COMMAND test_capi_debug)
