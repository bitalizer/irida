# SPDX-License-Identifier: BUSL-1.1
add_executable(test_proto_frame test_frame.cpp)
target_link_libraries(test_proto_frame PRIVATE irida_proto)
irida_set_warnings(test_proto_frame)
add_test(NAME proto.frame COMMAND test_proto_frame)
