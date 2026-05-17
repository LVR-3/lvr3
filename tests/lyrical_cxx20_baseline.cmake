if(NOT DEFINED LVR2_SOURCE_DIR)
  get_filename_component(LVR2_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

function(_lvr2_read _relative _out)
  set(_path "${LVR2_SOURCE_DIR}/${_relative}")
  if(NOT EXISTS "${_path}")
    message(FATAL_ERROR "Required baseline file is missing: ${_relative}")
  endif()
  file(READ "${_path}" _text)
  set(${_out} "${_text}" PARENT_SCOPE)
endfunction()

_lvr2_read("CMakeLists.txt" _root_cmake)
if(NOT _root_cmake MATCHES "cmake_minimum_required\\(VERSION 4\\.2")
  message(FATAL_ERROR "Top-level CMake minimum must target the Lyrical/Resolute CMake 4.2 baseline")
endif()

_lvr2_read("CMakePresets.json" _presets)
foreach(_required_json IN ITEMS "\"major\": 4" "\"minor\": 2")
  if(NOT _presets MATCHES "${_required_json}")
    message(FATAL_ERROR "CMakePresets.json must declare cmakeMinimumRequired 4.2")
  endif()
endforeach()

_lvr2_read("cmake/Lvr3ProjectSettings.cmake" _settings)
if(NOT _settings MATCHES "set\\(CMAKE_CXX_STANDARD 20\\)")
  message(FATAL_ERROR "Project settings must make C++20 the implementation default")
endif()
if(NOT _settings MATCHES "set\\(CMAKE_CXX_STANDARD_REQUIRED ON\\)")
  message(FATAL_ERROR "Project settings must require the selected C++ standard")
endif()
if(NOT _settings MATCHES "set\\(CMAKE_CXX_EXTENSIONS OFF\\)")
  message(FATAL_ERROR "Project settings must keep compiler extensions off")
endif()

_lvr2_read("src/liblvr2/CMakeLists.txt" _library_cmake)
foreach(_required_feature IN ITEMS
  "target_compile_features\\(lvr2core PRIVATE cxx_std_20\\)"
  "target_compile_features\\(lvr2_static PUBLIC cxx_std_20\\)"
  "target_compile_features\\(lvr2 PUBLIC cxx_std_20\\)"
  "target_compile_features\\(\\$\\{_LVR2_COMPONENT_TARGET\\} INTERFACE cxx_std_20\\)"
)
  if(NOT _library_cmake MATCHES "${_required_feature}")
    message(FATAL_ERROR "Library target graph is missing required C++20 feature export: ${_required_feature}")
  endif()
endforeach()

_lvr2_read("tests/package_identity/package_identity_consumer.cmake" _package_consumer)
if(NOT _package_consumer MATCHES "INTERFACE_COMPILE_FEATURES")
  message(FATAL_ERROR "Package identity smoke must verify exported C++20 compile features")
endif()
if(NOT _package_consumer MATCHES "cxx_std_20")
  message(FATAL_ERROR "Package identity smoke must require cxx_std_20")
endif()

_lvr2_read(".github/workflows/ci-smoke.yml" _github_ci)
foreach(_required_ci IN ITEMS "ros:lyrical-ros-base-resolute" "ubuntu-26.04" "lyrical_cxx20_baseline")
  if(NOT _github_ci MATCHES "${_required_ci}")
    message(FATAL_ERROR "GitHub CI must target Lyrical/Resolute and run the C++20 baseline guard: ${_required_ci}")
  endif()
endforeach()

_lvr2_read(".gitlab-ci.yml" _gitlab_ci)
foreach(_required_ci IN ITEMS "ros:lyrical-ros-base-resolute" "ubuntu:26.04" "lyrical_cxx20_baseline")
  if(NOT _gitlab_ci MATCHES "${_required_ci}")
    message(FATAL_ERROR "GitLab CI must target Lyrical/Resolute and run the C++20 baseline guard: ${_required_ci}")
  endif()
endforeach()

_lvr2_read("package.xml" _package_xml)
foreach(_required_package_token IN ITEMS "ROS 2 Lyrical" "Ubuntu Resolute 26.04" "C++20")
  string(FIND "${_package_xml}" "${_required_package_token}" _required_package_token_pos)
  if(_required_package_token_pos EQUAL -1)
    message(FATAL_ERROR "package.xml must document the active packaging lane: ${_required_package_token}")
  endif()
endforeach()

set(_baseline_scan_files
  CMakeLists.txt
  CMakePresets.json
  package.xml
  cmake/Lvr3ProjectSettings.cmake
  cmake/Lvr3Dependencies.cmake
  cmake/Lvr3Packaging.cmake
  src/liblvr2/CMakeLists.txt
  src/liblvr2/reconstruction/cuda/LBVHIndex.cu
  tests/CMakeLists.txt
  tests/packaging_policy.cmake
  tests/package_identity/package_identity_consumer.cmake
  tests/package_identity/package_identity_verify.cmake
  tests/test_logging_facade_header_compile.cpp
  static_assert_contracts.md
  README.md
  migration_guide.md
  docs/cmake/module-audit.md
  debian/README.Debian
  .github/workflows/ci-smoke.yml
  .gitlab-ci.yml
)
set(_baseline_banned_patterns
  "CMAKE_CXX_STANDARD[ \\t\\r\\n]+17"
  "CXX_STANDARD[ \\t\\r\\n]+17"
  "cxx_std_17"
  "-std=c\\+\\+17"
  "C\\+\\+17"
  "cmake_minimum_required\\(VERSION 3\\.22"
  "Ubuntu 18\\.04"
  "Ubuntu 20\\.04"
  "Ubuntu 22\\.04"
  "Ubuntu 24\\.04"
  "ubuntu-22\\.04"
  "ubuntu-24\\.04"
  "ubuntu:24\\.04"
  "ros:humble"
  "ros:jazzy"
  "Humble"
  "humble"
  "Jammy"
  "jammy"
  "Jazzy"
  "jazzy"
  "Noble"
  "noble"
)

foreach(_relative IN LISTS _baseline_scan_files)
  _lvr2_read("${_relative}" _scan_text)
  foreach(_pattern IN LISTS _baseline_banned_patterns)
    if(_scan_text MATCHES "${_pattern}")
      message(FATAL_ERROR "Stale pre-Lyrical/C++20 baseline token remains in ${_relative}: ${_pattern}")
    endif()
  endforeach()
endforeach()

message(STATUS "ROS 2 Lyrical / C++20 baseline guard passed")
