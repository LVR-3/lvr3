if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

foreach(_removed_viewer_path IN ITEMS
    src/tools/lvr2_viewer
    src/tools/lvr2_ascii_viewer
    CMakeModules/FindQVTK.cmake)
  if(EXISTS "${LVR2_SOURCE_DIR}/${_removed_viewer_path}")
    message(FATAL_ERROR "Bundled viewer artifact remains: ${_removed_viewer_path}")
  endif()
endforeach()

set(_viewer_cmake_files "${LVR2_SOURCE_DIR}/CMakeLists.txt")
file(GLOB _viewer_project_cmake_files
  LIST_DIRECTORIES false
  "${LVR2_SOURCE_DIR}/cmake/*.cmake"
  "${LVR2_SOURCE_DIR}/src/tools/CMakeLists.txt"
)
list(APPEND _viewer_cmake_files ${_viewer_project_cmake_files})
set(_root_cmake "")
foreach(_viewer_cmake_file IN LISTS _viewer_cmake_files)
  file(READ "${_viewer_cmake_file}" _viewer_cmake_text)
  string(APPEND _root_cmake "\n# ${_viewer_cmake_file}\n${_viewer_cmake_text}")
endforeach()
foreach(_forbidden_root_token IN ITEMS
    "LVR2_BUILD_VIEWER"
    "add_subdirectory(src/tools/lvr2_viewer)"
    "add_subdirectory(src/tools/lvr2_ascii_viewer)"
    "lvr2_find_package(Qt5"
    "lvr2_find_package(QVTK"
    "lvr2_find_package(Curses"
    "lvr2_find_package(VTK"
    "QVTK_OPENGL_WIDGET_INCLUDE_DIR"
    "VTK_USE_FILE")
  string(FIND "${_root_cmake}" "${_forbidden_root_token}" _token_pos)
  if(_token_pos GREATER_EQUAL 0)
    message(FATAL_ERROR "Viewer-only CMake token remains: ${_forbidden_root_token}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/vcpkg.json" _vcpkg_manifest)
foreach(_forbidden_manifest_token IN ITEMS "\"viewer\"" "\"qt5-base\"" "\"vtk\"")
  string(FIND "${_vcpkg_manifest}" "${_forbidden_manifest_token}" _manifest_pos)
  if(_manifest_pos GREATER_EQUAL 0)
    message(FATAL_ERROR "Viewer-only vcpkg manifest token remains: ${_forbidden_manifest_token}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/cmake/lvr2-config.cmake.in" _lvr2_config)
if(_lvr2_config MATCHES "find_dependency[ \\t\\r\\n]*\\([ \\t\\r\\n]*VTK")
  message(FATAL_ERROR "Installed package config must not advertise viewer-only VTK dependency")
endif()

foreach(_ci_file IN ITEMS .github/workflows/ci-smoke.yml .gitlab-ci.yml)
  if(EXISTS "${LVR2_SOURCE_DIR}/${_ci_file}")
    file(READ "${LVR2_SOURCE_DIR}/${_ci_file}" _ci_text)
    if(_ci_text MATCHES "LVR2_BUILD_VIEWER")
      message(FATAL_ERROR "CI file still passes removed viewer option: ${_ci_file}")
    endif()
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/debian/control" _debian_control)
foreach(_forbidden_debian_token IN ITEMS libvtk9-qt-dev libvtk6-qt-dev qtbase5-dev)
  if(_debian_control MATCHES "(^|[, \\n])${_forbidden_debian_token}([, \\n(]|$)")
    message(FATAL_ERROR "Debian packaging still depends on viewer-only package: ${_forbidden_debian_token}")
  endif()
endforeach()

message(STATUS "Bundled viewer removal guard passed")
