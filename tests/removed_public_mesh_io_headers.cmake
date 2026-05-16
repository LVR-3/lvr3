if(NOT DEFINED LVR2_SOURCE_DIR)
  get_filename_component(LVR2_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

set(_removed_public_headers
  "include/lvr2/mesh/io.hpp"
  "include/lvr2/io/ModelFactory.hpp"
  "include/lvr2/io/modelio/ObjIO.hpp"
  "include/lvr2/io/modelio/PLYIO.hpp"
  "include/lvr2/io/modelio/STLIO.hpp"
)

foreach(_header IN LISTS _removed_public_headers)
  if(EXISTS "${LVR2_SOURCE_DIR}/${_header}")
    message(FATAL_ERROR "Removed public mesh reader/writer header still exists: ${_header}")
  endif()
endforeach()

file(GLOB_RECURSE _public_headers
  "${LVR2_SOURCE_DIR}/include/lvr2/*.h"
  "${LVR2_SOURCE_DIR}/include/lvr2/*.hpp"
  "${LVR2_SOURCE_DIR}/include/lvr2/*.tcc"
)

set(_forbidden_includes
  "lvr2/mesh/io.hpp"
  "lvr2/io/ModelFactory.hpp"
  "lvr2/io/modelio/ObjIO.hpp"
  "lvr2/io/modelio/PLYIO.hpp"
  "lvr2/io/modelio/STLIO.hpp"
)

foreach(_public_header IN LISTS _public_headers)
  file(READ "${_public_header}" _content)
  foreach(_include IN LISTS _forbidden_includes)
    if(_content MATCHES "#[ \t]*include[ \t]*[<\"]${_include}[>\"]")
      file(RELATIVE_PATH _relative "${LVR2_SOURCE_DIR}" "${_public_header}")
      message(FATAL_ERROR "Public header ${_relative} includes removed mesh reader/writer header ${_include}")
    endif()
  endforeach()
endforeach()

message(STATUS "Removed public mesh reader/writer headers are absent from the public include tree")
