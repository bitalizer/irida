# SPDX-License-Identifier: BUSL-1.1
add_executable(test_capi_debug test_capi_debug.cpp)
target_link_libraries(test_capi_debug PRIVATE irida_capi)
irida_set_warnings(test_capi_debug)
add_test(NAME capi.debug COMMAND test_capi_debug)

# Compiles capi_target.cpp directly (see test_symres.cpp) so its unresolved
# externals (irida_session_create/destroy, defined in capi.cpp) need
# irida_capi itself, not just the libs capi_target.cpp normally builds against.
add_executable(test_symres test_symres.cpp)
target_include_directories(test_symres PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include
                                               ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_symres PRIVATE irida_capi irida_base irida_target irida_disasm
                                          irida_backend irida_binfmt)
if(WIN32)
  target_link_libraries(test_symres PRIVATE irida_host)
endif()
irida_set_warnings(test_symres)
add_test(NAME capi.symres COMMAND test_symres)
