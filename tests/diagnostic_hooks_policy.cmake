if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

set(_lvr2_cmake_policy_files
  "${LVR2_SOURCE_DIR}/CMakeLists.txt"
)
file(GLOB _lvr2_cmake_included_policy_files
  LIST_DIRECTORIES false
  "${LVR2_SOURCE_DIR}/cmake/*.cmake"
)
list(APPEND _lvr2_cmake_policy_files ${_lvr2_cmake_included_policy_files})
set(_lvr2_root_cmake "")
foreach(_lvr2_cmake_policy_file IN LISTS _lvr2_cmake_policy_files)
  file(READ "${_lvr2_cmake_policy_file}" _lvr2_cmake_policy_text)
  string(APPEND _lvr2_root_cmake "\n# ${_lvr2_cmake_policy_file}\n${_lvr2_cmake_policy_text}")
endforeach()
foreach(_lvr2_option IN ITEMS
    LVR2_ENABLE_SANITIZERS
    LVR2_ENABLE_FUZZING
    LVR2_FUZZ_WITH_LIBFUZZER
    LVR2_ENABLE_PERFORMANCE_BASELINES)
  if(NOT _lvr2_root_cmake MATCHES "option\\(${_lvr2_option}[^\n]*OFF\\)")
    message(FATAL_ERROR "${_lvr2_option} must exist and default to OFF")
  endif()
endforeach()

if(NOT _lvr2_root_cmake MATCHES [[LVR2_SANITIZERS[^\n]*address;undefined]])
  message(FATAL_ERROR "LVR2_SANITIZERS should default to address;undefined")
endif()

file(READ "${LVR2_SOURCE_DIR}/CMakePresets.json" _lvr2_presets)
foreach(_lvr2_preset IN ITEMS
    sanitizer-vcpkg-debug
    fuzz-vcpkg-debug
    performance-baseline-vcpkg-release)
  if(NOT _lvr2_presets MATCHES "\"name\"[ \t\r\n]*:[ \t\r\n]*\"${_lvr2_preset}\"")
    message(FATAL_ERROR "Missing optional preset ${_lvr2_preset}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/.github/workflows/ci-smoke.yml" _lvr2_github_ci)
foreach(_lvr2_expected IN ITEMS
    "run_optional_diagnostics"
    "continue-on-error: true"
    "optional diagnostics")
  if(NOT _lvr2_github_ci MATCHES "${_lvr2_expected}")
    message(FATAL_ERROR "GitHub CI is missing optional diagnostic marker: ${_lvr2_expected}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/.gitlab-ci.yml" _lvr2_gitlab_ci)
foreach(_lvr2_expected IN ITEMS
    "stage: diagnostics"
    "when: manual"
    "allow_failure: true")
  if(NOT _lvr2_gitlab_ci MATCHES "${_lvr2_expected}")
    message(FATAL_ERROR "GitLab CI is missing non-gating diagnostic marker: ${_lvr2_expected}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/docs/performance/baselines/smoke-baseline.json" _lvr2_baseline)
foreach(_lvr2_expected IN ITEMS
    "\"non_gating\": true"
    "\"schema_version\": 1"
    "generated_fixture_smoke")
  if(NOT _lvr2_baseline MATCHES "${_lvr2_expected}")
    message(FATAL_ERROR "Performance baseline JSON is missing marker: ${_lvr2_expected}")
  endif()
endforeach()
