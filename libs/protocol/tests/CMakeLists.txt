# SPDX-License-Identifier: BUSL-1.1
add_executable(test_proto_frame test_frame.cpp)
target_link_libraries(test_proto_frame PRIVATE irida_proto)
irida_set_warnings(test_proto_frame)
add_test(NAME proto.frame COMMAND test_proto_frame)

add_executable(test_proto_hex test_hex.cpp)
target_link_libraries(test_proto_hex PRIVATE irida_proto)
irida_set_warnings(test_proto_hex)
add_test(NAME proto.hex COMMAND test_proto_hex)

add_executable(test_proto_packets test_packets.cpp)
target_link_libraries(test_proto_packets PRIVATE irida_proto)
irida_set_warnings(test_proto_packets)
add_test(NAME proto.packets COMMAND test_proto_packets)
