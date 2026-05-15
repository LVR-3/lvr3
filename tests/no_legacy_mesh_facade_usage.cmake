if(NOT DEFINED LVR2_SOURCE_DIR OR LVR2_SOURCE_DIR STREQUAL "")
  message(FATAL_ERROR "LVR2_SOURCE_DIR must be provided")
endif()

set(_FACADE_FILE "${LVR2_SOURCE_DIR}/src/liblvr2/io/MeshIOFacade.cpp")
if(NOT EXISTS "${_FACADE_FILE}")
  message(FATAL_ERROR "Mesh facade implementation not found: ${_FACADE_FILE}")
endif()

file(READ "${_FACADE_FILE}" _FACADE_CONTENT)
set(_FORBIDDEN_FACADE_PATTERNS
  "ObjIO"
  "PLYIO"
  "STLIO"
  "ModelFactory"
  "ModelIOBase"
  "readWithLegacyIo"
  "saveWithLegacyIo"
  "LVR2_MESH_IO_HAS_ASSIMP"
)

foreach(_PATTERN IN LISTS _FORBIDDEN_FACADE_PATTERNS)
  if(_FACADE_CONTENT MATCHES "${_PATTERN}")
    message(FATAL_ERROR "Mesh facade still references legacy/optional I/O token '${_PATTERN}'")
  endif()
endforeach()

set(_REMOVED_PRIVATE_MESH_FILES
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/modelio/ObjIO.cpp"
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/modelio/STLIO.cpp"
  "${LVR2_SOURCE_DIR}/src/liblvr2/include/lvr2/io/modelio/ObjIO.hpp"
  "${LVR2_SOURCE_DIR}/src/liblvr2/include/lvr2/io/modelio/STLIO.hpp"
)
foreach(_FILE IN LISTS _REMOVED_PRIVATE_MESH_FILES)
  if(EXISTS "${_FILE}")
    message(FATAL_ERROR "Legacy mesh asset I/O file should be removed: ${_FILE}")
  endif()
endforeach()

set(_MODEL_FACTORY_FILE "${LVR2_SOURCE_DIR}/src/liblvr2/io/ModelFactory.cpp")
if(EXISTS "${_MODEL_FACTORY_FILE}")
  file(READ "${_MODEL_FACTORY_FILE}" _MODEL_FACTORY_CONTENT)
  foreach(_PATTERN "ObjIO" "STLIO")
    if(_MODEL_FACTORY_CONTENT MATCHES "${_PATTERN}")
      message(FATAL_ERROR "Private ModelFactory bridge still references removed mesh asset token '${_PATTERN}'")
    endif()
  endforeach()
  string(FIND "${_MODEL_FACTORY_CONTENT}" "mesh::load(filename, {mesh::Format::Ply})" _PLY_LOAD_FACADE)
  if(_PLY_LOAD_FACADE EQUAL -1)
    message(FATAL_ERROR "Private ModelFactory bridge must try the facade before retaining point-cloud PLY fallback")
  endif()
  string(FIND "${_MODEL_FACTORY_CONTENT}" "mesh::save(m->m_mesh, filename, {mesh::Format::Ply})" _PLY_SAVE_FACADE)
  if(_PLY_SAVE_FACADE EQUAL -1)
    message(FATAL_ERROR "Private ModelFactory bridge must route mesh PLY saves through the facade")
  endif()
  string(FIND "${_MODEL_FACTORY_CONTENT}" "meshResult.error().code == mesh::ErrorCode::ReadFailed" _PLY_POINT_CLOUD_FALLBACK)
  if(_PLY_POINT_CLOUD_FALLBACK EQUAL -1)
    message(FATAL_ERROR "Private ModelFactory bridge must retain point-cloud PLY fallback for backend read failures")
  endif()
endif()

message(STATUS "Mesh facade and private mesh bridge use required private backend without legacy mesh reader/writer paths")
