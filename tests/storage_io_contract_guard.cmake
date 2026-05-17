if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

foreach(_removed_path IN ITEMS
    "include/lvr2/io/baseio"
    "include/lvr2/io/meshio"
    "include/lvr2/io/scanio"
    "include/lvr2/io/deprecated/hdf5"
    "include/lvr2/io/ChunkIO.hpp"
    "src/liblvr2/io/ChunkIO.cpp"
    "src/liblvr2/io/HDF5IO.cpp"
    "src/liblvr2/io/scanio/LabelHDF5IO.cpp"
    "src/liblvr2/io/scanio/ScanProjectManager.cpp")
  if(EXISTS "${LVR2_SOURCE_DIR}/${_removed_path}")
    message(FATAL_ERROR "Removed storage feature path still exists: ${_removed_path}")
  endif()
endforeach()

foreach(_required_path IN ITEMS
    "include/lvr2/io/storage/StorageBackend.hpp"
    "include/lvr2/io/storage/ChunkStore.hpp"
    "include/lvr2/io/scan/ProjectStore.hpp"
    "src/liblvr2/private/lvr2/io/MeshStores.hpp"
    "src/liblvr2/io/storage/StorageRegistry.cpp"
    "src/liblvr2/io/storage/FileKernelStorageBackend.cpp"
    "src/liblvr2/io/scan/ProjectStore.cpp"
    "src/liblvr2/io/MeshStores.cpp"
    "src/liblvr2/io/ChunkStore.cpp")
  if(NOT EXISTS "${LVR2_SOURCE_DIR}/${_required_path}")
    message(FATAL_ERROR "Required single-path storage replacement file is missing: ${_required_path}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/include/lvr2/io/storage/StorageBackend.hpp" _storage_header)
foreach(_required_token IN ITEMS
    "namespace lvr2::io::storage"
    "class StorageBackend"
    "class StorageRegistry"
    "StorageFactory"
    "StorageContext"
    "#include <span>"
    "std::span<const std::byte>"
    "std::span<std::byte>"
    "TypedArrayView"
    "TypedDatasetView"
    "DatasetReader"
    "DatasetWriter"
    "StorageBackendLike"
    "StorageFactoryLike"
    "RegistryStorageFactory")
  string(FIND "${_storage_header}" "${_required_token}" _token_pos)
  if(_token_pos LESS 0)
    message(FATAL_ERROR "Storage backend replacement is missing token: ${_required_token}")
  endif()
endforeach()

foreach(_banned_storage_header_token IN ITEMS
    "#include <functional>"
    "std::function")
  string(FIND "${_storage_header}" "${_banned_storage_header_token}" _banned_storage_header_pos)
  if(_banned_storage_header_pos GREATER_EQUAL 0)
    message(FATAL_ERROR "Storage backend header still uses banned factory/storage token: ${_banned_storage_header_token}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/include/lvr2/io/scan/ProjectStore.hpp" _scan_header)
foreach(_required_token IN ITEMS
    "namespace lvr2::io::scan"
    "class ProjectStore"
    "open_project"
    "open_directory"
    "open_hdf5"
    "load_project"
    "save_project")
  string(FIND "${_scan_header}" "${_required_token}" _token_pos)
  if(_token_pos LESS 0)
    message(FATAL_ERROR "ProjectStore replacement is missing token: ${_required_token}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/src/liblvr2/io/ChunkStore.cpp" _chunk_store_source)
foreach(_required_token IN ITEMS
    "make_default_registry"
    "StorageKind::hdf5"
    "writePointBuffer"
    "readPointBuffer")
  string(FIND "${_chunk_store_source}" "${_required_token}" _token_pos)
  if(_token_pos LESS 0)
    message(FATAL_ERROR "ChunkStore compatibility path is missing StorageRegistry token: ${_required_token}")
  endif()
endforeach()

set(_lvr2_code_roots
  "${LVR2_SOURCE_DIR}/include"
  "${LVR2_SOURCE_DIR}/src"
  "${LVR2_SOURCE_DIR}/examples"
  "${LVR2_SOURCE_DIR}/tests"
)

set(_lvr2_banned_terms
  "BaseIO"
  "BASEIO"
  "FeatureBuild"
  "FeatureConstruct"
  "AddFeatures"
  "Merge<"
  "ScanProjectIO"
  "MatrixIO::"
  "ArrayIO::"
  "ScanIO::"
  "Hdf5Build"
  "Hdf5Construct"
  "hdf5features::"
  "Hdf5IO<"
  "BaseScanProjectIO"
  "lvr2/io/baseio"
  "lvr2/io/meshio"
  "lvr2/io/scanio"
  "lvr2/io/deprecated/hdf5"
  "namespace lvr2::scanio"
  "namespace lvr2::meshio"
  "namespace lvr2::modelio"
  "namespace lvr2::baseio"
  "namespace scanio"
  "namespace meshio"
  "namespace modelio"
  "namespace baseio"
  "lvr2::scanio"
  "lvr2::meshio"
  "lvr2::modelio"
  "lvr2::baseio"
  "built-in-only"
  "plugin-only"
  "custom-only"
  "separate custom"
)

foreach(_lvr2_root IN LISTS _lvr2_code_roots)
  if(IS_DIRECTORY "${_lvr2_root}")
    file(GLOB_RECURSE _lvr2_files
      "${_lvr2_root}/*.h"
      "${_lvr2_root}/*.hpp"
      "${_lvr2_root}/*.hh"
      "${_lvr2_root}/*.cpp"
      "${_lvr2_root}/*.cxx"
      "${_lvr2_root}/*.tcc"
      "${_lvr2_root}/*.cmake")
    foreach(_lvr2_file IN LISTS _lvr2_files)
      get_filename_component(_lvr2_name "${_lvr2_file}" NAME)
      if(_lvr2_name STREQUAL "storage_io_contract_guard.cmake" OR
         _lvr2_name STREQUAL "unified_io_namespace_guard.cmake")
        continue()
      endif()
      file(READ "${_lvr2_file}" _lvr2_text)
      foreach(_lvr2_banned IN LISTS _lvr2_banned_terms)
        string(FIND "${_lvr2_text}" "${_lvr2_banned}" _banned_pos)
        if(_banned_pos GREATER_EQUAL 0)
          file(RELATIVE_PATH _lvr2_relative "${LVR2_SOURCE_DIR}" "${_lvr2_file}")
          message(FATAL_ERROR "Removed storage feature token '${_lvr2_banned}' found in ${_lvr2_relative}")
        endif()
      endforeach()
    endforeach()
  endif()
endforeach()

foreach(_lvr2_cmake_file IN ITEMS
    "${LVR2_SOURCE_DIR}/CMakeLists.txt"
    "${LVR2_SOURCE_DIR}/src/liblvr2/CMakeLists.txt"
    "${LVR2_SOURCE_DIR}/tests/CMakeLists.txt")
  if(EXISTS "${_lvr2_cmake_file}")
    file(READ "${_lvr2_cmake_file}" _lvr2_text)
    foreach(_lvr2_banned IN LISTS _lvr2_banned_terms)
      string(FIND "${_lvr2_text}" "${_lvr2_banned}" _banned_pos)
      if(_banned_pos GREATER_EQUAL 0)
        file(RELATIVE_PATH _lvr2_relative "${LVR2_SOURCE_DIR}" "${_lvr2_cmake_file}")
        message(FATAL_ERROR "Removed storage feature token '${_lvr2_banned}' found in ${_lvr2_relative}")
      endif()
    endforeach()
  endif()
endforeach()

message(STATUS "Storage I/O final removal guard passed")
