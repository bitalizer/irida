# SPDX-License-Identifier: BUSL-1.1
add_executable(test_disasm test_disasm.cpp)
target_link_libraries(test_disasm PRIVATE irida_disasm)
irida_set_warnings(test_disasm)
add_test(NAME disasm.disasm COMMAND test_disasm)
