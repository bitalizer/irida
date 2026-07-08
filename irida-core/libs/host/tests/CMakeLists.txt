# SPDX-License-Identifier: BUSL-1.1
add_executable(test_host_tcp_loopback test_tcp_loopback.cpp)
target_link_libraries(test_host_tcp_loopback PRIVATE irida_host)
if(WIN32)
  target_link_libraries(test_host_tcp_loopback PRIVATE ws2_32)
endif()
irida_set_warnings(test_host_tcp_loopback)
add_test(NAME host.tcp_loopback COMMAND test_host_tcp_loopback)
