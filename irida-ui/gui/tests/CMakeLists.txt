# SPDX-License-Identifier: BUSL-1.1
find_package(Qt6 REQUIRED COMPONENTS Test Widgets)
set(CMAKE_AUTOMOC ON)

add_executable(test_table_view
  test_table_view.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/widgets/irida_table_view.cpp)
target_include_directories(test_table_view PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_table_view PRIVATE Qt6::Test Qt6::Widgets)
irida_set_warnings(test_table_view)
add_test(NAME gui.table_view COMMAND test_table_view)

add_executable(test_debug_controller
  test_debug_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/session/debug_controller.cpp)
target_include_directories(test_debug_controller PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_debug_controller PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets)
irida_set_warnings(test_debug_controller)
add_test(NAME gui.debug_controller COMMAND test_debug_controller)
