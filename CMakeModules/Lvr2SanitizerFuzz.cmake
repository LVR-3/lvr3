include_guard(GLOBAL)

function(_lvr2_collect_sanitizers out_var)
  set(_lvr2_supported_sanitizers address undefined leak thread memory)
  set(_lvr2_enabled_sanitizers "")

  foreach(_lvr2_sanitizer IN LISTS LVR2_SANITIZERS)
    string(STRIP "${_lvr2_sanitizer}" _lvr2_sanitizer)
    if(_lvr2_sanitizer STREQUAL "")
      continue()
    endif()
    string(TOLOWER "${_lvr2_sanitizer}" _lvr2_sanitizer)
    if(NOT _lvr2_sanitizer IN_LIST _lvr2_supported_sanitizers)
      message(FATAL_ERROR
        "Unsupported LVR2 sanitizer '${_lvr2_sanitizer}'. "
        "Supported values: ${_lvr2_supported_sanitizers}")
    endif()
    list(APPEND _lvr2_enabled_sanitizers "${_lvr2_sanitizer}")
  endforeach()

  list(REMOVE_DUPLICATES _lvr2_enabled_sanitizers)
  if(NOT _lvr2_enabled_sanitizers)
    message(FATAL_ERROR
      "LVR2_ENABLE_SANITIZERS is ON, but LVR2_SANITIZERS is empty. "
      "Use values such as 'address;undefined'.")
  endif()

  set(_lvr2_thread_sanitizer thread)
  set(_lvr2_address_sanitizer address)
  set(_lvr2_leak_sanitizer leak)
  set(_lvr2_memory_sanitizer memory)
  if(_lvr2_thread_sanitizer IN_LIST _lvr2_enabled_sanitizers)
    if(_lvr2_address_sanitizer IN_LIST _lvr2_enabled_sanitizers
       OR _lvr2_leak_sanitizer IN_LIST _lvr2_enabled_sanitizers
       OR _lvr2_memory_sanitizer IN_LIST _lvr2_enabled_sanitizers)
      message(FATAL_ERROR
        "ThreadSanitizer cannot be combined with address, leak, or memory sanitizers. "
        "Set LVR2_SANITIZERS=thread for a dedicated TSan build.")
    endif()
  endif()

  if(_lvr2_memory_sanitizer IN_LIST _lvr2_enabled_sanitizers
     AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "MemorySanitizer is only supported for Clang-family LVR2 builds.")
  endif()

  set(${out_var} ${_lvr2_enabled_sanitizers} PARENT_SCOPE)
endfunction()

function(lvr2_configure_sanitizer_hooks)
  if(NOT LVR2_ENABLE_SANITIZERS)
    return()
  endif()

  if(MSVC)
    message(FATAL_ERROR
      "LVR2_ENABLE_SANITIZERS currently supports GCC/Clang style -fsanitize builds only. "
      "Disable the option or use a Clang/GCC toolchain.")
  endif()

  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR
      "LVR2_ENABLE_SANITIZERS requires a compiler with GCC/Clang compatible -fsanitize flags; "
      "got '${CMAKE_CXX_COMPILER_ID}'.")
  endif()

  _lvr2_collect_sanitizers(_lvr2_enabled_sanitizers)
  string(JOIN "," _lvr2_sanitizer_arg ${_lvr2_enabled_sanitizers})

  message(STATUS "LVR2 sanitizer instrumentation enabled: ${_lvr2_sanitizer_arg}")
  add_compile_options(
    "$<$<COMPILE_LANGUAGE:C,CXX>:-fsanitize=${_lvr2_sanitizer_arg}>"
    "$<$<COMPILE_LANGUAGE:C,CXX>:-fno-omit-frame-pointer>"
  )
  add_link_options("-fsanitize=${_lvr2_sanitizer_arg}")
  add_compile_definitions(LVR2_SANITIZER_BUILD=1)
endfunction()

function(lvr2_configure_fuzz_target target_name)
  if(NOT TARGET ${target_name})
    message(FATAL_ERROR "lvr2_configure_fuzz_target called for missing target '${target_name}'.")
  endif()

  if(NOT LVR2_ENABLE_FUZZING)
    message(FATAL_ERROR
      "lvr2_configure_fuzz_target(${target_name}) requires LVR2_ENABLE_FUZZING=ON.")
  endif()

  target_compile_definitions(${target_name} PRIVATE LVR2_FUZZ_TARGET=1)

  if(LVR2_FUZZ_WITH_LIBFUZZER)
    if(MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      message(FATAL_ERROR
        "LVR2_FUZZ_WITH_LIBFUZZER requires a Clang-family compiler. "
        "Use -DLVR2_FUZZ_WITH_LIBFUZZER=OFF for the standalone seed-runner target.")
    endif()
    target_compile_options(${target_name} PRIVATE -fsanitize=fuzzer-no-link)
    target_link_options(${target_name} PRIVATE -fsanitize=fuzzer)
  else()
    target_compile_definitions(${target_name} PRIVATE LVR2_STANDALONE_FUZZ_DRIVER=1)
  endif()
endfunction()
