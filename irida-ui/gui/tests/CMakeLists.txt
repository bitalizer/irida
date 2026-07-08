# SPDX-License-Identifier: BUSL-1.1
find_package(Qt6 REQUIRED COMPONENTS Test Widgets Svg)
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

add_executable(test_panels_core
  test_panels_core.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/widgets/irida_table_view.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/session/debug_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/disassembly/disassembly_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/registers/registers_panel.cpp)
target_include_directories(test_panels_core PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_panels_core PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets)
irida_set_warnings(test_panels_core)
add_test(NAME gui.panels_core COMMAND test_panels_core)

add_executable(test_panels_mem
  test_panels_mem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/widgets/irida_table_view.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/session/debug_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/memory/memory_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/memory/stack_panel.cpp)
target_include_directories(test_panels_mem PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_panels_mem PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets)
irida_set_warnings(test_panels_mem)
add_test(NAME gui.panels_mem COMMAND test_panels_mem)

add_executable(test_panels_side
  test_panels_side.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/widgets/irida_table_view.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/session/debug_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/symbols/modules_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/execution/breakpoints_panel.cpp)
target_include_directories(test_panels_side PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_panels_side PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets)
irida_set_warnings(test_panels_side)
add_test(NAME gui.panels_side COMMAND test_panels_side)

add_executable(test_cpu_widget
  test_cpu_widget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/widgets/irida_table_view.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/session/debug_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/disassembly/disassembly_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/registers/registers_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/memory/memory_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/memory/stack_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/layouts/cpu_widget.cpp)
target_include_directories(test_cpu_widget PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_cpu_widget PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets)
irida_set_warnings(test_cpu_widget)
add_test(NAME gui.cpu_widget COMMAND test_cpu_widget)

add_executable(test_icons
  test_icons.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/theme/icons.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons.qrc)
target_include_directories(test_icons PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_icons PRIVATE Qt6::Test Qt6::Widgets Qt6::Svg)
set_target_properties(test_icons PROPERTIES AUTORCC ON)
irida_set_warnings(test_icons)
add_test(NAME gui.icons COMMAND test_icons)

add_executable(test_main_window
  test_main_window.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/app/main_window.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/theme/icons.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/session/debug_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/widgets/irida_table_view.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/layouts/cpu_widget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/disassembly/disassembly_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/registers/registers_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/memory/memory_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/memory/stack_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/symbols/modules_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/panels/execution/breakpoints_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons.qrc)
target_include_directories(test_main_window PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_main_window PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets Qt6::Svg)
set_target_properties(test_main_window PROPERTIES AUTORCC ON)
irida_set_warnings(test_main_window)
add_test(NAME gui.main_window COMMAND test_main_window)
