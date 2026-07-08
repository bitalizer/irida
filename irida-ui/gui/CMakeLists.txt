# SPDX-License-Identifier: BUSL-1.1
find_package(Qt6 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)

add_executable(irida_gui
  src/main.cpp
  src/main_window.cpp
  src/disasm_view.cpp)
target_link_libraries(irida_gui PRIVATE irida_capi irida_mock Qt6::Widgets)
irida_set_warnings(irida_gui)
