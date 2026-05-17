if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

set(_roots
  include
  src
  examples
  cmake
  CMakeLists.txt
  CMakePresets.json
  Singularity.def
  vcpkg.json
  package.xml
  debian
  .github
  .gitlab-ci.yml
)

set(_patterns
  "#[ \t]*include[ \t]*[<\"]boost/"
  "boost::"
  "BOOST_[A-Za-z0-9_]+"
  "find_(package|dependency)[ \t]*\\([ \t]*Boost"
  "Boost_(COMPONENTS|LIBRARIES|INCLUDE_DIRS|LIBRARY_DIR|LOG_LIBRARY|DIAGNOSTIC_DEFINITIONS)"
  "boost-[A-Za-z0-9.+-]+"
  "libboost-[A-Za-z0-9.+-]+"
  "<depend>boost</depend>"
)

set(_matches "")
foreach(_root IN LISTS _roots)
  set(_path "${LVR2_SOURCE_DIR}/${_root}")
  if(IS_DIRECTORY "${_path}")
    file(GLOB_RECURSE _files LIST_DIRECTORIES false "${_path}/*")
  elseif(EXISTS "${_path}")
    set(_files "${_path}")
  else()
    set(_files "")
  endif()

  foreach(_file IN LISTS _files)
    file(RELATIVE_PATH _rel "${LVR2_SOURCE_DIR}" "${_file}")
    file(READ "${_file}" _content)
    foreach(_pattern IN LISTS _patterns)
      if(_content MATCHES "${_pattern}")
        string(APPEND _matches "${_rel}: ${_pattern}\n")
      endif()
    endforeach()
  endforeach()
endforeach()

if(_matches)
  message(FATAL_ERROR "Boost dependency tokens remain in active LVR surfaces:\n${_matches}")
endif()

message(STATUS "No Boost dependency tokens remain in active LVR surfaces")
