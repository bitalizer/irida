# SPDX-License-Identifier: BUSL-1.1
find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(irida_gui WIN32
  src/app/main.cpp
  src/app/main_window.cpp
  src/dialogs/attach_proc_dialog.cpp
  src/dialogs/welcome_dialog.cpp
  src/dialogs/load_options_dialog.cpp
  src/theme/icons.cpp
  src/theme/palette.cpp
  src/theme/asm_tokenizer.cpp
  src/theme/asm_item_delegate.cpp
  src/theme/gutter_delegate.cpp
  src/session/debug_controller.cpp
  src/widgets/irida_table_view.cpp
  src/layouts/cpu_widget.cpp
  src/panels/disassembly/disassembly_panel.cpp
  src/panels/registers/registers_panel.cpp
  src/panels/memory/memory_panel.cpp
  src/panels/memory/memory_map_panel.cpp
  src/panels/memory/stack_panel.cpp
  src/panels/symbols/modules_panel.cpp
  src/panels/binfmt/sections_panel.cpp
  src/panels/binfmt/imports_panel.cpp
  src/panels/binfmt/exports_panel.cpp
  src/panels/binfmt/symbols_panel.cpp
  src/panels/binfmt/strings_panel.cpp
  src/panels/analysis/functions_panel.cpp
  src/panels/analysis/xrefs_panel.cpp
  src/panels/execution/breakpoints_panel.cpp
  src/panels/execution/threads_panel.cpp
  src/panels/console/console_panel.cpp
  src/panels/execution/backtrace_panel.cpp
  src/panels/graph/graph_view.cpp
  resources/icons.qrc)
target_include_directories(irida_gui PRIVATE src)
target_link_libraries(irida_gui PRIVATE irida_capi irida_mock Qt6::Widgets Qt6::Svg)
irida_set_warnings(irida_gui)

# /SUBSYSTEM:WINDOWS marks the PE as GUI (no attached console); /ENTRY overrides
# the subsystem's default WinMain entry back to the CRT startup that calls main().
if(MSVC)
  set_target_properties(irida_gui PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

add_subdirectory(tests)
