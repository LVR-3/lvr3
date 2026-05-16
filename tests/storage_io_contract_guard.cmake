if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

set(_lvr2_inventory_doc "${LVR2_SOURCE_DIR}/docs/io/baseio-removal-inventory.md")
if(NOT EXISTS "${_lvr2_inventory_doc}")
  message(FATAL_ERROR "Missing BaseIO removal inventory: ${_lvr2_inventory_doc}")
endif()

file(READ "${_lvr2_inventory_doc}" _lvr2_inventory_text)

foreach(_required_section IN ITEMS
    "Replacement contracts"
    "Term inventory"
    "Complete file inventory"
    "Public and bundled migration map"
    "Scan-project smoke commands"
    "Performance baseline commands")
  string(FIND "${_lvr2_inventory_text}" "${_required_section}" _lvr2_section_pos)
  if(_lvr2_section_pos LESS 0)
    message(FATAL_ERROR "BaseIO inventory is missing required section: ${_required_section}")
  endif()
endforeach()

foreach(_required_token IN ITEMS
    "BaseIO"
    "FeatureBuild"
    "FeatureConstruct"
    "AddFeatures"
    "Merge<"
    "ScanProjectIO::"
    "DirectoryIO"
    "HDF5IO"
    "lvr2::io::storage::StorageRegistry"
    "lvr2::io::storage::StorageBackend"
    "lvr2::io::scan"
    "ProjectStore"
    "open_directory"
    "open_hdf5"
    "load_project"
    "save_project"
    "fake/test"
    "performance-baselines")
  string(FIND "${_lvr2_inventory_text}" "${_required_token}" _lvr2_token_pos)
  if(_lvr2_token_pos LESS 0)
    message(FATAL_ERROR "BaseIO inventory is missing required token: ${_required_token}")
  endif()
endforeach()

# Future replacement code should appear under these roots, not under the legacy
# scanio/meshio/baseio CRTP namespaces. When those roots exist, keep them free of
# the CRTP vocabulary and closed-set backend-selection shortcuts.
set(_lvr2_replacement_roots
  "${LVR2_SOURCE_DIR}/include/lvr2/io/storage"
  "${LVR2_SOURCE_DIR}/include/lvr2/io/scan"
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/storage"
  "${LVR2_SOURCE_DIR}/src/liblvr2/io/scan"
)

set(_lvr2_banned_replacement_terms
  "BaseIO"
  "FeatureBuild"
  "FeatureConstruct"
  "AddFeatures"
  "Merge<"
  "ScanProjectIO::"
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
  "std::variant"
  "built-in-only"
  "plugin-only"
  "custom-only"
  "separate custom"
)

foreach(_lvr2_root IN LISTS _lvr2_replacement_roots)
  if(IS_DIRECTORY "${_lvr2_root}")
    file(GLOB_RECURSE _lvr2_replacement_files
      "${_lvr2_root}/*.h"
      "${_lvr2_root}/*.hpp"
      "${_lvr2_root}/*.hh"
      "${_lvr2_root}/*.cpp"
      "${_lvr2_root}/*.tcc")
    foreach(_lvr2_file IN LISTS _lvr2_replacement_files)
      file(READ "${_lvr2_file}" _lvr2_file_text)
      foreach(_lvr2_banned IN LISTS _lvr2_banned_replacement_terms)
        string(FIND "${_lvr2_file_text}" "${_lvr2_banned}" _lvr2_banned_pos)
        if(_lvr2_banned_pos GREATER_EQUAL 0)
          message(FATAL_ERROR
            "Replacement storage/scan code must not contain '${_lvr2_banned}': ${_lvr2_file}")
        endif()
      endforeach()
    endforeach()
  endif()
endforeach()
