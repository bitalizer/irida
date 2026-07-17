# SPDX-License-Identifier: BUSL-1.1
find_package(Qt6 REQUIRED COMPONENTS Test Widgets Svg)
set(CMAKE_AUTOMOC ON)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Theme sources are pulled in wherever irida_table_view.cpp is compiled, since
# the base view now paints via the theme palette + delegates.
set(THEME_SRC
  ${SRC}/theme/palette.cpp
  ${SRC}/theme/asm_tokenizer.cpp
  ${SRC}/theme/asm_item_delegate.cpp
  ${SRC}/theme/gutter_delegate.cpp)

add_executable(test_asm_tokenizer
  test_asm_tokenizer.cpp
  ${SRC}/theme/asm_tokenizer.cpp)
target_include_directories(test_asm_tokenizer PRIVATE ${SRC})
target_link_libraries(test_asm_tokenizer PRIVATE Qt6::Test)
irida_set_warnings(test_asm_tokenizer)
add_test(NAME gui.asm_tokenizer COMMAND test_asm_tokenizer)

add_executable(test_table_view
  test_table_view.cpp
  ${SRC}/widgets/irida_table_view.cpp
  ${THEME_SRC})
target_include_directories(test_table_view PRIVATE ${SRC})
target_link_libraries(test_table_view PRIVATE Qt6::Test Qt6::Widgets)
irida_set_warnings(test_table_view)
add_test(NAME gui.table_view COMMAND test_table_view)

add_executable(test_debug_controller
  test_debug_controller.cpp
  ${SRC}/session/debug_controller.cpp)
target_include_directories(test_debug_controller PRIVATE
  ${SRC}
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_debug_controller PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets)
irida_set_warnings(test_debug_controller)
add_test(NAME gui.debug_controller COMMAND test_debug_controller)

add_executable(test_panels_core
  test_panels_core.cpp
  ${SRC}/widgets/irida_table_view.cpp
  ${THEME_SRC}
  ${SRC}/session/debug_controller.cpp
  ${SRC}/panels/disassembly/disassembly_panel.cpp
  ${SRC}/panels/registers/registers_panel.cpp)
target_include_directories(test_panels_core PRIVATE
  ${SRC}
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_panels_core PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets)
irida_set_warnings(test_panels_core)
add_test(NAME gui.panels_core COMMAND test_panels_core)

add_executable(test_panels_mem
  test_panels_mem.cpp
  ${SRC}/widgets/irida_table_view.cpp
  ${THEME_SRC}
  ${SRC}/session/debug_controller.cpp
  ${SRC}/panels/memory/memory_panel.cpp
  ${SRC}/panels/memory/stack_panel.cpp)
target_include_directories(test_panels_mem PRIVATE
  ${SRC}
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_panels_mem PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets)
irida_set_warnings(test_panels_mem)
add_test(NAME gui.panels_mem COMMAND test_panels_mem)

add_executable(test_panels_side
  test_panels_side.cpp
  ${SRC}/widgets/irida_table_view.cpp
  ${THEME_SRC}
  ${SRC}/session/debug_controller.cpp
  ${SRC}/panels/symbols/modules_panel.cpp
  ${SRC}/panels/execution/breakpoints_panel.cpp)
target_include_directories(test_panels_side PRIVATE
  ${SRC}
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_panels_side PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets)
irida_set_warnings(test_panels_side)
add_test(NAME gui.panels_side COMMAND test_panels_side)

add_executable(test_cpu_widget
  test_cpu_widget.cpp
  ${SRC}/widgets/irida_table_view.cpp
  ${THEME_SRC}
  ${SRC}/session/debug_controller.cpp
  ${SRC}/panels/disassembly/disassembly_panel.cpp
  ${SRC}/panels/registers/registers_panel.cpp
  ${SRC}/panels/memory/memory_panel.cpp
  ${SRC}/panels/memory/stack_panel.cpp
  ${SRC}/layouts/cpu_widget.cpp)
target_include_directories(test_cpu_widget PRIVATE
  ${SRC}
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_cpu_widget PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets)
irida_set_warnings(test_cpu_widget)
add_test(NAME gui.cpu_widget COMMAND test_cpu_widget)

add_executable(test_icons
  test_icons.cpp
  ${SRC}/theme/icons.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons.qrc)
target_include_directories(test_icons PRIVATE ${SRC})
target_link_libraries(test_icons PRIVATE Qt6::Test Qt6::Widgets Qt6::Svg)
set_target_properties(test_icons PROPERTIES AUTORCC ON)
irida_set_warnings(test_icons)
add_test(NAME gui.icons COMMAND test_icons)

add_executable(test_main_window
  test_main_window.cpp
  ${SRC}/app/main_window.cpp
  ${SRC}/theme/icons.cpp
  ${THEME_SRC}
  ${SRC}/session/debug_controller.cpp
  ${SRC}/widgets/irida_table_view.cpp
  ${SRC}/layouts/cpu_widget.cpp
  ${SRC}/panels/disassembly/disassembly_panel.cpp
  ${SRC}/panels/registers/registers_panel.cpp
  ${SRC}/panels/memory/memory_panel.cpp
  ${SRC}/panels/memory/stack_panel.cpp
  ${SRC}/panels/symbols/modules_panel.cpp
  ${SRC}/panels/execution/breakpoints_panel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons.qrc)
target_include_directories(test_main_window PRIVATE
  ${SRC}
  ${CMAKE_SOURCE_DIR}/irida-api/include
  ${CMAKE_SOURCE_DIR}/irida-ui/mock/include)
target_link_libraries(test_main_window PRIVATE irida_capi irida_mock Qt6::Test Qt6::Widgets Qt6::Svg)
set_target_properties(test_main_window PROPERTIES AUTORCC ON)
irida_set_warnings(test_main_window)
add_test(NAME gui.main_window COMMAND test_main_window)
