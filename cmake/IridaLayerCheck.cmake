# SPDX-License-Identifier: BUSL-1.1
# Assert allowed direct dependencies per target. Fails configure on violation.
# Allowed edges (values may only be linked BY the key):
#   irida_base       -> (none)
#   irida_proto      -> irida_base
#   irida_host       -> irida_base  (+ ws2_32 system lib, not checked)
#   irida_transport  -> irida_proto, irida_host, irida_base
#   irida_backend    -> irida_transport, irida_proto, irida_host, irida_base
#   irida_target     -> irida_backend, irida_proto, irida_base
#   irida_disasm     -> irida_base, Zydis::Zydis
#   irida_binfmt     -> irida_base, LIEF::LIEF
#   irida_capi       -> irida_base, irida_target, irida_disasm, irida_backend, irida_proto,
#                       irida_host (WIN32 only, via native_backend.hpp)
#   irida_mock       -> irida_capi
#   irida_gui        -> irida_capi, irida_mock, Qt6::Widgets, Qt6::Svg
function(irida_assert_deps target)
  set(allowed ${ARGN})
  get_target_property(links ${target} LINK_LIBRARIES)
  if(NOT links)
    return()
  endif()
  foreach(dep IN LISTS links)
    # Ignore non-irida deps except Qt (explicitly allowed where listed).
    if(dep MATCHES "^irida_" OR dep MATCHES "^Qt6::")
      list(FIND allowed "${dep}" idx)
      if(idx EQUAL -1)
        message(FATAL_ERROR
          "Layer violation: target '${target}' links '${dep}' which is not in its allowed set (${allowed}).")
      endif()
    endif()
  endforeach()
endfunction()

function(irida_run_layer_check)
  irida_assert_deps(irida_proto irida_base)
  irida_assert_deps(irida_host irida_base ws2_32)
  irida_assert_deps(irida_transport irida_proto irida_host irida_base)
  irida_assert_deps(irida_backend irida_transport irida_proto irida_host irida_base)
  irida_assert_deps(irida_target irida_backend irida_proto irida_base)
  irida_assert_deps(irida_disasm irida_base Zydis::Zydis)
  irida_assert_deps(irida_binfmt irida_base LIEF::LIEF)
  irida_assert_deps(irida_capi irida_base irida_target irida_disasm irida_backend irida_proto irida_host)
  irida_assert_deps(irida_mock irida_capi)
  if(TARGET irida_gui)
    irida_assert_deps(irida_gui irida_capi irida_mock Qt6::Widgets Qt6::Svg)
  endif()
endfunction()
