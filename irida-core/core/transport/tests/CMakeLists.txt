# SPDX-License-Identifier: BUSL-1.1
add_executable(test_gdb_client test_gdb_client.cpp mock_gdb_server.cpp)
target_link_libraries(test_gdb_client PRIVATE irida_transport irida_proto)
if(WIN32)
  target_link_libraries(test_gdb_client PRIVATE ws2_32)
endif()
irida_set_warnings(test_gdb_client)
add_test(NAME transport.gdb_client COMMAND test_gdb_client)
