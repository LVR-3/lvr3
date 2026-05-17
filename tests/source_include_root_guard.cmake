if(NOT DEFINED LVR2_SOURCE_DIR OR LVR2_SOURCE_DIR STREQUAL "")
  message(FATAL_ERROR "LVR2_SOURCE_DIR must be provided")
endif()

foreach(_removed_root IN ITEMS
    "src/liblvr2/include")
  if(EXISTS "${LVR2_SOURCE_DIR}/${_removed_root}")
    message(FATAL_ERROR "Removed source include root still exists: ${_removed_root}")
  endif()
endforeach()

file(GLOB _lvr2_tool_include_roots
  LIST_DIRECTORIES true
  "${LVR2_SOURCE_DIR}/src/tools/*/include")
foreach(_lvr2_tool_include_root IN LISTS _lvr2_tool_include_roots)
  if(IS_DIRECTORY "${_lvr2_tool_include_root}")
    file(RELATIVE_PATH _lvr2_relative "${LVR2_SOURCE_DIR}" "${_lvr2_tool_include_root}")
    message(FATAL_ERROR "Removed tool source include root still exists: ${_lvr2_relative}")
  endif()
endforeach()

foreach(_required_private_header IN ITEMS
    "src/liblvr2/private/lvr2/io/AssimpMeshAdapter.hpp"
    "src/liblvr2/private/lvr2/io/MeshStores.hpp"
    "src/liblvr2/private/lvr2/io/ModelFactory.hpp"
    "src/liblvr2/private/lvr2/io/modelio/PLYIO.hpp"
    "src/tools/lvr2_hdf5_convert_old/private/Hdf5ReaderOld.hpp"
    "src/tools/lvr2_hdf5_convert_old/private/ScanTypesCompare.hpp")
  if(NOT EXISTS "${LVR2_SOURCE_DIR}/${_required_private_header}")
    message(FATAL_ERROR "Expected private header is missing: ${_required_private_header}")
  endif()
endforeach()

set(_lvr2_scan_roots
  "${LVR2_SOURCE_DIR}/cmake"
  "${LVR2_SOURCE_DIR}/include"
  "${LVR2_SOURCE_DIR}/src"
  "${LVR2_SOURCE_DIR}/tests"
  "${LVR2_SOURCE_DIR}/examples")

set(_lvr2_source_files "${LVR2_SOURCE_DIR}/CMakeLists.txt")
foreach(_lvr2_root IN LISTS _lvr2_scan_roots)
  if(IS_DIRECTORY "${_lvr2_root}")
    file(GLOB_RECURSE _lvr2_root_files
      "${_lvr2_root}/CMakeLists.txt"
      "${_lvr2_root}/*.cmake"
      "${_lvr2_root}/*.h"
      "${_lvr2_root}/*.hpp"
      "${_lvr2_root}/*.hh"
      "${_lvr2_root}/*.cpp"
      "${_lvr2_root}/*.cxx"
      "${_lvr2_root}/*.tcc")
    list(APPEND _lvr2_source_files ${_lvr2_root_files})
  endif()
endforeach()

list(REMOVE_DUPLICATES _lvr2_source_files)
set(_lvr2_self "${LVR2_SOURCE_DIR}/tests/source_include_root_guard.cmake")

foreach(_lvr2_file IN LISTS _lvr2_source_files)
  if("${_lvr2_file}" STREQUAL "${_lvr2_self}")
    continue()
  endif()
  file(READ "${_lvr2_file}" _lvr2_text)
  foreach(_lvr2_banned IN ITEMS
      "src/liblvr2/include"
      "src/tools/lvr2_hdf5_convert_old/include")
    string(FIND "${_lvr2_text}" "${_lvr2_banned}" _lvr2_banned_pos)
    if(_lvr2_banned_pos GREATER_EQUAL 0)
      file(RELATIVE_PATH _lvr2_relative "${LVR2_SOURCE_DIR}" "${_lvr2_file}")
      message(FATAL_ERROR "Removed source include root token '${_lvr2_banned}' found in ${_lvr2_relative}")
    endif()
  endforeach()

  string(REGEX MATCH "(^|[\r\n])[ \t]*include_directories[ \t\r\n]*\\([^)]*src/" _lvr2_global_src_include "${_lvr2_text}")
  if(_lvr2_global_src_include)
    file(RELATIVE_PATH _lvr2_relative "${LVR2_SOURCE_DIR}" "${_lvr2_file}")
    message(FATAL_ERROR "Global source include_directories shortcut found in ${_lvr2_relative}")
  endif()

  if("${_lvr2_file}" MATCHES "/src/")
    string(FIND "${_lvr2_text}" [[${CMAKE_CURRENT_SOURCE_DIR}/include]] _lvr2_current_source_include_pos)
    string(REGEX MATCH "(^|[\r\n])[ \t]*include_directories[ \t\r\n]*\\([ \t\r\n]*include[ \t\r\n]*\\)" _lvr2_relative_source_include "${_lvr2_text}")
    if(_lvr2_current_source_include_pos GREATER_EQUAL 0 OR _lvr2_relative_source_include)
      file(RELATIVE_PATH _lvr2_relative "${LVR2_SOURCE_DIR}" "${_lvr2_file}")
      message(FATAL_ERROR "Source-local include root shortcut found in ${_lvr2_relative}")
    endif()
  endif()
endforeach()

message(STATUS "Source include-root guard passed")
