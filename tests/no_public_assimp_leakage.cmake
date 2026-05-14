if(NOT DEFINED LVR2_SOURCE_DIR OR LVR2_SOURCE_DIR STREQUAL "")
  message(FATAL_ERROR "LVR2_SOURCE_DIR must be provided")
endif()

set(_PUBLIC_PATHS
  "${LVR2_SOURCE_DIR}/include/lvr2"
  "${LVR2_SOURCE_DIR}/CMakeModules/lvr2-config.cmake.in"
  "${LVR2_SOURCE_DIR}/CMakeModules/lvr3-config.cmake.in"
)

set(_FORBIDDEN_PATTERNS
  "assimp/"
  "Assimp"
  "aiScene"
  "aiMesh"
  "aiMaterial"
  "Importer"
  "Exporter"
  "find_dependency\\(assimp"
  "assimp::"
  "ASSIMP"
  "LVR2_MESH_IO_HAS_ASSIMP"
)

foreach(_PATH IN LISTS _PUBLIC_PATHS)
  if(IS_DIRECTORY "${_PATH}")
    file(GLOB_RECURSE _FILES "${_PATH}/*")
  else()
    set(_FILES "${_PATH}")
  endif()

  foreach(_FILE IN LISTS _FILES)
    if(NOT IS_DIRECTORY "${_FILE}")
      file(READ "${_FILE}" _CONTENT)
      foreach(_PATTERN IN LISTS _FORBIDDEN_PATTERNS)
        if(_CONTENT MATCHES "${_PATTERN}")
          message(FATAL_ERROR "Public header/package config leaks private backend token '${_PATTERN}' in ${_FILE}")
        endif()
      endforeach()
    endif()
  endforeach()
endforeach()

message(STATUS "No private backend tokens found in public headers or package config templates")
