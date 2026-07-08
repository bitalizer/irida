# SPDX-License-Identifier: BUSL-1.1
# Applies the project's warning policy and C++ standard to a target.
function(irida_set_warnings target)
  target_compile_features(${target} PUBLIC cxx_std_20)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endfunction()
