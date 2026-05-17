if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

set(_expected_modules
  FindFLANN.cmake
  FindLZ4.cmake
  FindRDB.cmake
)

file(GLOB _actual_module_paths
  LIST_DIRECTORIES false
  "${LVR2_SOURCE_DIR}/cmake/modules/*.cmake"
)
set(_actual_modules "")
foreach(_module_path IN LISTS _actual_module_paths)
  get_filename_component(_module_name "${_module_path}" NAME)
  list(APPEND _actual_modules "${_module_name}")
endforeach()
list(SORT _actual_modules)
list(SORT _expected_modules)

if(NOT "${_actual_modules}" STREQUAL "${_expected_modules}")
  message(FATAL_ERROR
    "cmake/modules must contain only audited local finder exceptions.\n"
    "Expected: ${_expected_modules}\n"
    "Actual: ${_actual_modules}")
endif()

set(_forbidden_modules
  FindDraco.cmake
  FindGEOTIFF.cmake
  FindOpenCL2.cmake
  FindOpenNI.cmake
  FindOpenNI2.cmake
  Findembree.cmake
  TBBConfig.cmake
  Lvr2DependencyPolicy.cmake
  Lvr2SanitizerFuzz.cmake
  lvr2-config.cmake.in
  lvr2-packaging.cmake
  lvr2-uninstall.cmake.in
  lvr3-config.cmake.in
  max_cuda_gcc_version.cmake
)
foreach(_forbidden_module IN LISTS _forbidden_modules)
  if(EXISTS "${LVR2_SOURCE_DIR}/CMakeModules/${_forbidden_module}" OR EXISTS "${LVR2_SOURCE_DIR}/cmake/modules/${_forbidden_module}")
    message(FATAL_ERROR "Removed or relocated CMakeModules file remains: ${_forbidden_module}")
  endif()
endforeach()

set(_required_project_cmake_files
  cmake/Lvr3DependencyProvider.cmake
  cmake/Lvr3Options.cmake
  cmake/Lvr3DependencyPolicy.cmake
  cmake/Lvr3ProjectSettings.cmake
  cmake/Lvr3Testing.cmake
  cmake/Lvr3Dependencies.cmake
  cmake/Lvr3Targets.cmake
  cmake/Lvr3Install.cmake
  cmake/Lvr3Documentation.cmake
  cmake/Lvr3Packaging.cmake
  cmake/Lvr3SanitizerFuzz.cmake
  cmake/Lvr3CudaGccVersion.cmake
  cmake/modules/FindFLANN.cmake
  cmake/modules/FindLZ4.cmake
  cmake/modules/FindRDB.cmake
  cmake/modules/README.md
  cmake/lvr2-config.cmake.in
  cmake/lvr3-config.cmake.in
  cmake/lvr2-uninstall.cmake.in
  src/tools/CMakeLists.txt
  docs/cmake/module-audit.md
)
foreach(_required_file IN LISTS _required_project_cmake_files)
  if(NOT EXISTS "${LVR2_SOURCE_DIR}/${_required_file}")
    message(FATAL_ERROR "Required split CMake/audit file is missing: ${_required_file}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/docs/cmake/module-audit.md" _audit_doc)
foreach(_module IN LISTS _expected_modules)
  if(NOT _audit_doc MATCHES "`${_module}`")
    message(FATAL_ERROR "docs/cmake/module-audit.md does not document ${_module}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/CMakeLists.txt" _root_cmake)
foreach(_required_include IN ITEMS
    "cmake/Lvr3DependencyProvider.cmake"
    "cmake/Lvr3DependencyPolicy.cmake"
    "cmake/Lvr3Options.cmake"
    "cmake/Lvr3ProjectSettings.cmake"
    "cmake/Lvr3Testing.cmake"
    "cmake/Lvr3Dependencies.cmake"
    "cmake/Lvr3Targets.cmake"
    "cmake/Lvr3Install.cmake"
    "cmake/Lvr3Documentation.cmake"
    "cmake/Lvr3Packaging.cmake")
  if(NOT _root_cmake MATCHES "include\\([^\\n]*${_required_include}")
    message(FATAL_ERROR "Root CMakeLists.txt must include ${_required_include}")
  endif()
endforeach()

foreach(_forbidden_root_token IN ITEMS
    "lvr2_find_package(FLANN"
    "lvr2_find_package(LZ4"
    "add_subdirectory(src/tools/lvr2_reconstruct)"
    "configure_package_config_file(cmake/lvr2-config.cmake.in")
  string(FIND "${_root_cmake}" "${_forbidden_root_token}" _forbidden_root_token_pos)
  if(_forbidden_root_token_pos GREATER_EQUAL 0)
    message(FATAL_ERROR "Root CMakeLists.txt still owns split responsibility token: ${_forbidden_root_token}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/cmake/Lvr3Dependencies.cmake" _dependencies_cmake)
foreach(_forbidden_dependency_token IN ITEMS
    "lvr2_find_package(Draco"
    "lvr2_find_package(OpenCL2")
  string(FIND "${_dependencies_cmake}" "${_forbidden_dependency_token}" _forbidden_dependency_token_pos)
  if(_forbidden_dependency_token_pos GREATER_EQUAL 0)
    message(FATAL_ERROR "Replaceable local finder dependency remains: ${_forbidden_dependency_token}")
  endif()
endforeach()
foreach(_required_dependency_token IN ITEMS
    "lvr2_find_package(draco CONFIG QUIET)"
    "lvr2_find_package(OpenCL QUIET)"
    "set(OpenCL_LIBRARIES OpenCL::OpenCL)")
  string(FIND "${_dependencies_cmake}" "${_required_dependency_token}" _required_dependency_token_pos)
  if(_required_dependency_token_pos LESS 0)
    message(FATAL_ERROR "Expected config/standard dependency lookup is missing: ${_required_dependency_token}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/cmake/lvr2-config.cmake.in" _lvr2_config_template)
foreach(_required_export_dependency_token IN ITEMS
    "find_dependency(draco CONFIG)"
    "find_dependency(OpenCL)")
  string(FIND "${_lvr2_config_template}" "${_required_export_dependency_token}" _required_export_dependency_token_pos)
  if(_required_export_dependency_token_pos LESS 0)
    message(FATAL_ERROR "Optional config/standard dependency is missing from installed package config: ${_required_export_dependency_token}")
  endif()
endforeach()
string(FIND "${_lvr2_config_template}" "include(\${CMAKE_CURRENT_LIST_DIR}/lvr2-targets.cmake)" _targets_include_pos)
foreach(_required_pre_target_dependency_token IN ITEMS
    "find_dependency(draco CONFIG)"
    "find_dependency(OpenCL)")
  string(FIND "${_lvr2_config_template}" "${_required_pre_target_dependency_token}" _pre_target_dependency_pos)
  if(_targets_include_pos LESS 0 OR _pre_target_dependency_pos GREATER _targets_include_pos)
    message(FATAL_ERROR "Optional imported-target dependency must be found before lvr2-targets.cmake: ${_required_pre_target_dependency_token}")
  endif()
endforeach()

if(EXISTS "${LVR2_SOURCE_DIR}/CMakeModules")
  message(FATAL_ERROR "The legacy CMakeModules directory must be removed; use cmake/ and cmake/modules instead")
endif()

message(STATUS "CMake module audit guard passed")
