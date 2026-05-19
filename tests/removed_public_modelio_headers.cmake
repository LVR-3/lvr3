if(NOT DEFINED LVR2_SOURCE_DIR)
  get_filename_component(LVR2_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

set(_modelio_public_dir "${LVR2_SOURCE_DIR}/include/lvr2/io/modelio")
if(EXISTS "${_modelio_public_dir}")
  message(FATAL_ERROR "Old modelio reader/writer headers must not be installed as public API: include/lvr2/io/modelio")
endif()

set(_removed_public_headers
  "include/lvr2/io/AsciiIO.hpp"
  "include/lvr2/io/B3dmIO.hpp"
  "include/lvr2/io/DatIO.hpp"
  "include/lvr2/io/DracoDecoder.hpp"
  "include/lvr2/io/DracoEncoder.hpp"
  "include/lvr2/io/DrcIO.hpp"
  "include/lvr2/io/GeoTIFFIO.hpp"
  "include/lvr2/io/LasIO.hpp"
  "include/lvr2/io/ModelIOBase.hpp"
  "include/lvr2/io/PCDIO.hpp"
  "include/lvr2/io/PLYIO.hpp"
  "include/lvr2/io/PPMIO.hpp"
  "include/lvr2/io/RdbxIO.hpp"
  "include/lvr2/io/RxpIO.hpp"
  "include/lvr2/io/UosIO.hpp"
  "include/lvr2/io/WaveformIO.hpp"
)

foreach(_header IN LISTS _removed_public_headers)
  if(EXISTS "${LVR2_SOURCE_DIR}/${_header}")
    message(FATAL_ERROR "Removed public modelio reader/writer header still exists: ${_header}")
  endif()
endforeach()

file(GLOB_RECURSE _public_headers
  "${LVR2_SOURCE_DIR}/include/lvr2/*.h"
  "${LVR2_SOURCE_DIR}/include/lvr2/*.hpp"
  "${LVR2_SOURCE_DIR}/include/lvr2/*.tcc"
)

set(_removed_root_header_pattern "#[ \t]*include[ \t]*[<\"]lvr2/io/(AsciiIO|B3dmIO|DatIO|DracoDecoder|DracoEncoder|DrcIO|GeoTIFFIO|LasIO|ModelIOBase|PCDIO|PLYIO|PPMIO|RdbxIO|RxpIO|UosIO|WaveformIO)\\.hpp[>\"]")

foreach(_public_header IN LISTS _public_headers)
  file(READ "${_public_header}" _content)
  file(RELATIVE_PATH _relative "${LVR2_SOURCE_DIR}" "${_public_header}")
  if(_content MATCHES "lvr2/io/modelio/")
    message(FATAL_ERROR "Public header ${_relative} includes old modelio/private reader path")
  endif()
  if(_content MATCHES "${_removed_root_header_pattern}")
    message(FATAL_ERROR "Public header ${_relative} includes removed root modelio reader/writer header")
  endif()
endforeach()

message(STATUS "Removed public modelio reader/writer headers are absent from the public include tree")
