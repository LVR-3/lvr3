if(NOT DEFINED LVR2_SOURCE_DIR)
  get_filename_component(LVR2_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

set(_dependency_policy_vendored_dirs
  ext/CTPL
  ext/fmt
  ext/HighFive
  ext/QVTKOpenGLWidget
  ext/kintinuous
  ext/laslib
  ext/nanoflann
  ext/psimpl
  ext/rply
  ext/spdlog
  ext/spdmon
)

foreach(_dependency_policy_dir IN LISTS _dependency_policy_vendored_dirs)
  if(EXISTS "${LVR2_SOURCE_DIR}/${_dependency_policy_dir}")
    message(FATAL_ERROR "Vendored dependency directory remains: ${_dependency_policy_dir}")
  endif()
endforeach()

if(NOT EXISTS "${LVR2_SOURCE_DIR}/vcpkg.json")
  message(FATAL_ERROR "The dependency policy requires a vcpkg manifest at vcpkg.json")
endif()

set(_dependency_policy_cmake_files
  "${LVR2_SOURCE_DIR}/CMakeLists.txt"
  "${LVR2_SOURCE_DIR}/src/liblvr2/CMakeLists.txt"
  "${LVR2_SOURCE_DIR}/src/tools/CMakeLists.txt"
  "${LVR2_SOURCE_DIR}/tests/CMakeLists.txt"
)
file(GLOB _dependency_policy_project_cmake_files
  LIST_DIRECTORIES false
  "${LVR2_SOURCE_DIR}/cmake/*.cmake"
  "${LVR2_SOURCE_DIR}/cmake/modules/*.cmake"
)
list(APPEND _dependency_policy_cmake_files ${_dependency_policy_project_cmake_files})
file(GLOB_RECURSE _dependency_policy_nested_cmake_files
  LIST_DIRECTORIES false
  "${LVR2_SOURCE_DIR}/src/tools/*/CMakeLists.txt"
  "${LVR2_SOURCE_DIR}/examples/*/CMakeLists.txt"
  "${LVR2_SOURCE_DIR}/examples/*/*/CMakeLists.txt"
)
list(APPEND _dependency_policy_cmake_files ${_dependency_policy_nested_cmake_files})

set(_dependency_policy_forbidden_patterns
  "add_subdirectory[ \\t\\r\\n]*\\([ \\t\\r\\n]*ext/"
  "include_directories[ \\t\\r\\n]*\\([^\\)]*ext/"
  "install[ \\t\\r\\n]*\\([^\\)]*ext/"
  "ExternalProject_Add[ \\t\\r\\n]*\\("
  "HIGHFIVE_INCLUDE_DIRS"
  "NANOFLANN_INCLUDE_DIR"
  "PSIMPL_INCLUDE_DIR"
  "CTPL_INCLUDE_DIR"
  "QVTK_PATCHED_INCLUDE_DIR"
)

foreach(_dependency_policy_file IN LISTS _dependency_policy_cmake_files)
  if(EXISTS "${_dependency_policy_file}")
    file(READ "${_dependency_policy_file}" _dependency_policy_text)
    foreach(_dependency_policy_pattern IN LISTS _dependency_policy_forbidden_patterns)
      if(_dependency_policy_text MATCHES "${_dependency_policy_pattern}")
        file(RELATIVE_PATH _dependency_policy_rel "${LVR2_SOURCE_DIR}" "${_dependency_policy_file}")
        message(FATAL_ERROR "Vendored dependency CMake pattern remains in ${_dependency_policy_rel}: ${_dependency_policy_pattern}")
      endif()
    endforeach()
  endif()
endforeach()

message(STATUS "Vendored dependency guard passed")
