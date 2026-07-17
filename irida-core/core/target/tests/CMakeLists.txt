# SPDX-License-Identifier: BUSL-1.1
set(IRIDA_BACKEND_TEST_INCLUDE ${CMAKE_SOURCE_DIR}/irida-core/core/backend/tests)

add_executable(test_register_set test_register_set.cpp)
target_link_libraries(test_register_set PRIVATE irida_target)
irida_set_warnings(test_register_set)
add_test(NAME target.register_set COMMAND test_register_set)

# MemoryCache now reads through irida::backend::Backend, so it's exercised here
# via the MockBackend test double (irida-core/core/backend/tests) instead of a
# scripted gdb server/GdbClient — zero dependency on gdb transport or make_gdb_backend().
add_executable(test_memory_cache test_memory_cache.cpp)
target_include_directories(test_memory_cache PRIVATE ${IRIDA_BACKEND_TEST_INCLUDE})
target_link_libraries(test_memory_cache PRIVATE irida_target irida_backend)
irida_set_warnings(test_memory_cache)
add_test(NAME target.memory_cache COMMAND test_memory_cache)

# Target is driven via an injected MockBackend (Target::attach(unique_ptr<Backend>)).
add_executable(test_target test_target.cpp)
target_include_directories(test_target PRIVATE ${IRIDA_BACKEND_TEST_INCLUDE})
target_link_libraries(test_target PRIVATE irida_target irida_backend)
irida_set_warnings(test_target)
add_test(NAME target.target COMMAND test_target)

add_executable(test_backend_smoke test_backend_smoke.cpp)
target_include_directories(test_backend_smoke PRIVATE ${IRIDA_BACKEND_TEST_INCLUDE})
target_link_libraries(test_backend_smoke PRIVATE irida_target irida_backend)
irida_set_warnings(test_backend_smoke)
add_test(NAME target.backend_smoke COMMAND test_backend_smoke)
