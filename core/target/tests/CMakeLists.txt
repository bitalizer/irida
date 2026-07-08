# SPDX-License-Identifier: BUSL-1.1
add_executable(test_register_set test_register_set.cpp)
target_link_libraries(test_register_set PRIVATE irida_target)
irida_set_warnings(test_register_set)
add_test(NAME target.register_set COMMAND test_register_set)

add_executable(test_memory_cache test_memory_cache.cpp scripted_gdb_server.cpp)
target_link_libraries(test_memory_cache PRIVATE irida_target irida_transport irida_proto)
if(WIN32)
  target_link_libraries(test_memory_cache PRIVATE ws2_32)
endif()
irida_set_warnings(test_memory_cache)
add_test(NAME target.memory_cache COMMAND test_memory_cache)
