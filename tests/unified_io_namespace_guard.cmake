if(NOT DEFINED LVR2_SOURCE_DIR OR LVR2_SOURCE_DIR STREQUAL "")
  message(FATAL_ERROR "LVR2_SOURCE_DIR must be provided")
endif()

set(_lvr2_new_mesh_header "${LVR2_SOURCE_DIR}/include/lvr2/io/mesh.hpp")
set(_lvr2_old_mesh_header "${LVR2_SOURCE_DIR}/include/lvr2/mesh/io.hpp")

if(NOT EXISTS "${_lvr2_new_mesh_header}")
  message(FATAL_ERROR "Unified mesh I/O header is missing: include/lvr2/io/mesh.hpp")
endif()
if(EXISTS "${_lvr2_old_mesh_header}")
  message(FATAL_ERROR "Old public mesh facade header must not be installed: include/lvr2/mesh/io.hpp")
endif()

file(READ "${_lvr2_new_mesh_header}" _lvr2_mesh_header_text)
foreach(_lvr2_required IN ITEMS
    "namespace lvr2::io::mesh"
    "Result<lvr2::MeshBufferPtr> load"
    "Status save")
  string(FIND "${_lvr2_mesh_header_text}" "${_lvr2_required}" _lvr2_required_pos)
  if(_lvr2_required_pos LESS 0)
    message(FATAL_ERROR "Unified mesh header is missing required token: ${_lvr2_required}")
  endif()
endforeach()

set(_lvr2_code_roots
  "${LVR2_SOURCE_DIR}/include"
  "${LVR2_SOURCE_DIR}/src"
  "${LVR2_SOURCE_DIR}/examples"
  "${LVR2_SOURCE_DIR}/tests"
)

foreach(_lvr2_root IN LISTS _lvr2_code_roots)
  if(IS_DIRECTORY "${_lvr2_root}")
    file(GLOB_RECURSE _lvr2_code_files
      "${_lvr2_root}/*.h"
      "${_lvr2_root}/*.hpp"
      "${_lvr2_root}/*.hh"
      "${_lvr2_root}/*.cpp"
      "${_lvr2_root}/*.cxx"
      "${_lvr2_root}/*.tcc")
    foreach(_lvr2_file IN LISTS _lvr2_code_files)
      file(READ "${_lvr2_file}" _lvr2_text)
      foreach(_lvr2_old_mesh IN ITEMS
          "lvr2/mesh/io.hpp"
          "lvr2::mesh::")
        string(FIND "${_lvr2_text}" "${_lvr2_old_mesh}" _lvr2_old_mesh_pos)
        if(_lvr2_old_mesh_pos GREATER_EQUAL 0)
          file(RELATIVE_PATH _lvr2_relative "${LVR2_SOURCE_DIR}" "${_lvr2_file}")
          message(FATAL_ERROR "Old mesh facade token '${_lvr2_old_mesh}' found in ${_lvr2_relative}")
        endif()
      endforeach()

      # Guard/test files may contain banned legacy namespace names as policy
      # strings. Live C++ source must not declare or use those namespaces.
      foreach(_lvr2_old_namespace IN ITEMS
          "namespace meshio"
          "namespace scanio"
          "namespace modelio"
          "namespace baseio"
          "namespace lvr2::meshio"
          "namespace lvr2::scanio"
          "namespace lvr2::modelio"
          "namespace lvr2::baseio"
          "meshio::"
          "scanio::"
          "modelio::"
          "baseio::")
        string(FIND "${_lvr2_text}" "${_lvr2_old_namespace}" _lvr2_old_namespace_pos)
        if(_lvr2_old_namespace_pos GREATER_EQUAL 0)
          file(RELATIVE_PATH _lvr2_relative "${LVR2_SOURCE_DIR}" "${_lvr2_file}")
          message(FATAL_ERROR "Live legacy I/O namespace token '${_lvr2_old_namespace}' found in ${_lvr2_relative}")
        endif()
      endforeach()
    endforeach()
  endif()
endforeach()

# Final replacement root scan: the old split storage feature headers are gone;
# new lvr2::io replacement roots must not carry public split namespaces or the
# old feature lattice forward.
set(_lvr2_replacement_files
  "${LVR2_SOURCE_DIR}/include/lvr2/io/mesh.hpp"
  "${LVR2_SOURCE_DIR}/include/lvr2/io/storage/ChunkStore.hpp"
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/MeshIOFacade.cpp"
  "${LVR2_SOURCE_DIR}/src/liblvr2/private/lvr2/io/AssimpMeshAdapter.hpp"
  "${LVR2_SOURCE_DIR}/src/liblvr2/private/lvr2/io/MeshStores.hpp"
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/AssimpMeshAdapter.cpp"
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/MeshStores.cpp"
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/ChunkStore.cpp"
)

set(_lvr2_replacement_roots
  "${LVR2_SOURCE_DIR}/include/lvr2/io/mesh"
  "${LVR2_SOURCE_DIR}/include/lvr2/io/scan"
  "${LVR2_SOURCE_DIR}/include/lvr2/io/storage"
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/mesh"
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/scan"
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/storage"
)

foreach(_lvr2_root IN LISTS _lvr2_replacement_roots)
  if(IS_DIRECTORY "${_lvr2_root}")
    file(GLOB_RECURSE _lvr2_root_files
      "${_lvr2_root}/*.h"
      "${_lvr2_root}/*.hpp"
      "${_lvr2_root}/*.hh"
      "${_lvr2_root}/*.cpp"
      "${_lvr2_root}/*.cxx"
      "${_lvr2_root}/*.tcc")
    list(APPEND _lvr2_replacement_files ${_lvr2_root_files})
  endif()
endforeach()

set(_lvr2_banned_replacement_terms
  "BaseIO"
  "FeatureBuild"
  "FeatureConstruct"
  "AddFeatures"
  "Merge<"
  "ScanProjectIO"
  "MatrixIO::"
  "ArrayIO::"
  "ScanIO::"
  "namespace meshio"
  "namespace scanio"
  "namespace modelio"
  "namespace baseio"
  "namespace lvr2::meshio"
  "namespace lvr2::scanio"
  "namespace lvr2::modelio"
  "namespace lvr2::baseio"
  "lvr2::meshio"
  "lvr2::scanio"
  "lvr2::modelio"
  "lvr2::baseio"
  "std::variant"
  "built-in-only"
  "plugin-only"
  "custom-only"
  "separate custom"
)

foreach(_lvr2_file IN LISTS _lvr2_replacement_files)
  if(EXISTS "${_lvr2_file}")
    file(READ "${_lvr2_file}" _lvr2_text)
    foreach(_lvr2_banned IN LISTS _lvr2_banned_replacement_terms)
      string(FIND "${_lvr2_text}" "${_lvr2_banned}" _lvr2_banned_pos)
      if(_lvr2_banned_pos GREATER_EQUAL 0)
        file(RELATIVE_PATH _lvr2_relative "${LVR2_SOURCE_DIR}" "${_lvr2_file}")
        message(FATAL_ERROR "Unified I/O replacement root contains banned token '${_lvr2_banned}': ${_lvr2_relative}")
      endif()
    endforeach()
  endif()
endforeach()

message(STATUS "Unified I/O namespace guard passed")
