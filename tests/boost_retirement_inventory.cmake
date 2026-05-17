if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

set(_inventory "${LVR2_SOURCE_DIR}/docs/dependencies/boost-removal-inventory.md")
if(NOT EXISTS "${_inventory}")
  message(FATAL_ERROR "Missing Boost retirement inventory: ${_inventory}")
endif()

file(READ "${_inventory}" _content)
foreach(_token IN ITEMS
  "Status: complete for active LVR-owned build, package, and implementation surfaces"
  "Expected result: no matches outside historical docs and Boost-specific policy guard names"
  "lvr2::util::MappedFile"
  "Boost property-tree XML parsing"
  "Boost.MPI package path"
  "Generic Boost package/export plumbing"
  "lvr2_no_boost_dependency"
  "Future Boost reintroduction requires a new ADR-approved exception"
)
  if(NOT _content MATCHES "${_token}")
    message(FATAL_ERROR "Boost retirement inventory is missing required token: ${_token}")
  endif()
endforeach()

message(STATUS "Boost retirement inventory records completed hard-case removal")
