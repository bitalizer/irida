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
