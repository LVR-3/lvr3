if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

set(_inventory "${LVR2_SOURCE_DIR}/docs/dependencies/boost-removal-inventory.md")
if(NOT EXISTS "${_inventory}")
  message(FATAL_ERROR "Missing Boost retirement inventory: ${_inventory}")
endif()

file(READ "${_inventory}" _content)

set(_required_tokens
  "Active files with Boost tokens: 21"
  "Public headers with active Boost tokens: 3"
  "directives: 10 across 7 distinct Boost headers"
  "Active `boost::...` references: 14"
  "Build/package/CI Boost references: 41"
  "standard-equivalent Boost families have been replaced"
  "filesystem | `std::filesystem`"
  "optional | `std::optional`"
  "variant / visitor | `std::variant` + `std::visit`"
  "shared_array | `std::shared_ptr<T[]>` owner + `std::span` view"
  "boost.system | `std::error_code` / filesystem errors"
  "program_options | LVR-owned typed CLI parser"
  "`boost::iostreams::mapped_file`"
  "Boost archive serialization"
  "Boost property-tree XML parsing"
  "Boost.MPI packaging"
  "DateTime / Boost.Log link remnants"
  "No active `boost/mpi` or `boost::mpi` code was found"
  "cmake/Lvr3Dependencies.cmake"
  "vcpkg.json"
  "package.xml"
  "debian/control"
)

foreach(_token IN LISTS _required_tokens)
  string(FIND "${_content}" "${_token}" _found)
  if(_found EQUAL -1)
    message(FATAL_ERROR "Boost retirement inventory is missing required token: ${_token}")
  endif()
endforeach()
