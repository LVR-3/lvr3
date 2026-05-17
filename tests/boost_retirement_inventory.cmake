if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

set(_inventory "${LVR2_SOURCE_DIR}/docs/dependencies/boost-removal-inventory.md")
if(NOT EXISTS "${_inventory}")
  message(FATAL_ERROR "Missing Boost retirement inventory: ${_inventory}")
endif()

file(READ "${_inventory}" _content)

set(_required_tokens
  "Active files with Boost tokens: 224"
  "Public headers with active Boost tokens: 108"
  "directives: 191 across 30 distinct Boost headers"
  "Active `boost::...` references: 968"
  "Build/package/CI Boost references: 67"
  "standard-library replacement"
  "buffer ownership/span replacement"
  "CLI parser replacement"
  "hard-case storage/reconstruction cleanup"
  "final build/package dependency cleanup"
  "filesystem | `std::filesystem`"
  "optional | `std::optional`"
  "variant / visitor | `std::variant` + `std::visit`"
  "shared_array | explicit owner + `std::span` view"
  "program_options | LVR-owned typed CLI parser"
  "iostreams / mapped file | byte-span memory-map adapter or simpler file I/O"
  "property_tree | explicit XML/config parser"
  "boost.system | `std::error_code` / filesystem errors"
  "DateTime / Boost.Log link remnants"
  "Boost.MPI packaging"
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
