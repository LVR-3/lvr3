if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

set(_roots
  include
  src
  cmake
  package.xml
  vcpkg.json
  debian
  .github
  .gitlab-ci.yml
)

set(_matches "")
foreach(_root IN LISTS _roots)
  set(_path "${LVR2_SOURCE_DIR}/${_root}")
  if(EXISTS "${_path}")
    if(IS_DIRECTORY "${_path}")
      file(GLOB_RECURSE _files CONFIGURE_DEPENDS
        "${_path}/*")
    else()
      set(_files "${_path}")
    endif()
    foreach(_file IN LISTS _files)
      if(IS_DIRECTORY "${_file}")
        continue()
      endif()
      file(READ "${_file}" _content)
      if(_content MATCHES "boost/program_options|boost::program_options|Boost_PROGRAM_OPTIONS|boost-program-options|libboost-program-options|program_options")
        string(APPEND _matches "${_file}\n")
      endif()
    endforeach()
  endif()
endforeach()

if(_matches)
  message(FATAL_ERROR "Boost program option tokens remain:\n${_matches}")
endif()
