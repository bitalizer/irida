# SPDX-License-Identifier: BUSL-1.1
add_executable(irida-cli src/main.cpp)
target_link_libraries(irida-cli PRIVATE irida_capi)
irida_set_warnings(irida-cli)
