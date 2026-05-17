if(NOT DEFINED LVR2_SOURCE_DIR)
  get_filename_component(LVR2_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

set(_logging_policy_roots
  include
  src
  examples
  tests
)

set(_logging_policy_banned_patterns
  "logout::get[ \t\r\n]*\\("
  "Logger::get[ \t\r\n]*\\("
  "\\bLoggerEndline\\b"
  "\\bLoggerError\\b"
  "\\bLoggerWarning\\b"
  "\\bLoggerInfo\\b"
  "\\bLoggerDebug\\b"
  "\\bLoggerTrace\\b"
  "class[ \t\r\n]+Logger\\b"
  "struct[ \t\r\n]+Logger\\b"
  "using[ \t\r\n]+logout[ \t\r\n]*="
  "lvr2::Logger\\b"
  "\\bLogger[ \t\r\n]*&"
  "lvr2::endl\\b"
  "fmt::"
  "#[ \t]*include[ \t]*[<\"]fmt/"
  "FMT_"
  "LVR2_LOG_"
  "\\bLOG[ \t\r\n]*\\."
  "LOG[ \t\r\n]*\\([ \t\r\n]*Logger::"
  "std::cout[ \t\r\n]*<<[ \t\r\n]*(lvr2::)?timestamp\\b"
  "std::cout[^;\n]*<<[ \t\r\n]*(lvr2::)?timestamp\\b"
  "(^|[^A-Za-z0-9_:])cout[ \t\r\n]*<<[ \t\r\n]*(lvr2::)?timestamp\\b"
  "(^|[^A-Za-z0-9_:])cout[^;\n]*<<[ \t\r\n]*(lvr2::)?timestamp\\b"
  "std::cerr[ \t\r\n]*<<[ \t\r\n]*(lvr2::)?timestamp\\b"
  "std::cerr[^;\n]*<<[ \t\r\n]*(lvr2::)?timestamp\\b"
  "(^|[^A-Za-z0-9_:])cerr[ \t\r\n]*<<[ \t\r\n]*(lvr2::)?timestamp\\b"
  "(^|[^A-Za-z0-9_:])cerr[^;\n]*<<[ \t\r\n]*(lvr2::)?timestamp\\b"
)

foreach(_logging_policy_root IN LISTS _logging_policy_roots)
  file(GLOB_RECURSE _logging_policy_files
    LIST_DIRECTORIES false
    "${LVR2_SOURCE_DIR}/${_logging_policy_root}/*.h"
    "${LVR2_SOURCE_DIR}/${_logging_policy_root}/*.hh"
    "${LVR2_SOURCE_DIR}/${_logging_policy_root}/*.hpp"
    "${LVR2_SOURCE_DIR}/${_logging_policy_root}/*.hxx"
    "${LVR2_SOURCE_DIR}/${_logging_policy_root}/*.tcc"
    "${LVR2_SOURCE_DIR}/${_logging_policy_root}/*.cpp"
    "${LVR2_SOURCE_DIR}/${_logging_policy_root}/*.cc"
    "${LVR2_SOURCE_DIR}/${_logging_policy_root}/*.cxx"
    "${LVR2_SOURCE_DIR}/${_logging_policy_root}/*.cu"
    "${LVR2_SOURCE_DIR}/${_logging_policy_root}/*.cmake"
  )
  foreach(_logging_policy_file IN LISTS _logging_policy_files)
    file(RELATIVE_PATH _logging_policy_rel "${LVR2_SOURCE_DIR}" "${_logging_policy_file}")
    if(_logging_policy_rel STREQUAL "tests/format_logging_policy.cmake")
      continue()
    endif()
    file(READ "${_logging_policy_file}" _logging_policy_text)
    foreach(_logging_policy_pattern IN LISTS _logging_policy_banned_patterns)
      if(_logging_policy_text MATCHES "${_logging_policy_pattern}")
        message(FATAL_ERROR
          "Removed fmt/macro/stream logging pattern remains in ${_logging_policy_rel}: ${_logging_policy_pattern}")
      endif()
    endforeach()
  endforeach()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/include/lvr2/util/Logging.hpp" _logging_policy_header)
foreach(_required_token IN ITEMS
  "namespace log"
  "std::format_string"
  "std::source_location"
  "class SourceLogger"
  "SourceLogger here"
  "logger_handle"
  "write_runtime"
  "SPDLOG_USE_STD_FORMAT"
)
  if(NOT _logging_policy_header MATCHES "${_required_token}")
    message(FATAL_ERROR "Logging facade header is missing required token '${_required_token}'")
  endif()
endforeach()

foreach(_banned_header_token IN ITEMS
  "fmt::"
  "#[ \t]*include[ \t]*[<\"]fmt/"
  "FMT_"
  "LVR2_LOG_"
)
  if(_logging_policy_header MATCHES "${_banned_header_token}")
    message(FATAL_ERROR "Logging facade must not retain fmt or logging macros: ${_banned_header_token}")
  endif()
endforeach()

foreach(_public_signature_leak IN ITEMS
  "void[ \t\r\n]+(trace|debug|info|warning|warn|error|write)[^;{\n]*(fmt::|spdlog::)"
)
  if(_logging_policy_header MATCHES "${_public_signature_leak}")
    message(FATAL_ERROR "Stable public logging signatures must not mention fmt:: or spdlog::")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/migration_guide.md" _logging_policy_migration_guide)
foreach(_banned_migration_token IN ITEMS "fmt::" "LVR2_LOG_" "find_dependency(fmt CONFIG)")
  string(FIND "${_logging_policy_migration_guide}" "${_banned_migration_token}" _banned_migration_pos)
  if(NOT _banned_migration_pos EQUAL -1)
    message(FATAL_ERROR "Public logging migration guide must not retain removed fmt/macro guidance: ${_banned_migration_token}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/cmake/Lvr3Dependencies.cmake" _logging_policy_dependencies)
foreach(_required_dependency_token IN ITEMS "lvr2_find_package(spdlog CONFIG REQUIRED)" "-DSPDLOG_USE_STD_FORMAT" "spdlog::spdlog_header_only")
  string(FIND "${_logging_policy_dependencies}" "${_required_dependency_token}" _required_dependency_pos)
  if(_required_dependency_pos EQUAL -1)
    message(FATAL_ERROR "Build dependency setup must configure spdlog standard formatting: ${_required_dependency_token}")
  endif()
endforeach()
foreach(_banned_dependency_token IN ITEMS "lvr2_find_package(fmt" "LVR2_FMT_TARGET" "set(LVR2_SPDLOG_TARGET spdlog::spdlog)")
  string(FIND "${_logging_policy_dependencies}" "${_banned_dependency_token}" _banned_dependency_pos)
  if(NOT _banned_dependency_pos EQUAL -1)
    message(FATAL_ERROR "Build dependency setup must not retain fmt dependency wiring: ${_banned_dependency_token}")
  endif()
endforeach()

foreach(_config_template IN ITEMS cmake/lvr2-config.cmake.in cmake/lvr3-config.cmake.in)
  file(READ "${LVR2_SOURCE_DIR}/${_config_template}" _logging_policy_config)
  string(FIND "${_logging_policy_config}" "find_dependency(spdlog CONFIG)" _required_spdlog_config_pos)
  if(_required_spdlog_config_pos EQUAL -1)
    message(FATAL_ERROR "Installed packages must declare spdlog for the inline logging detail layer: ${_config_template}")
  endif()
  string(FIND "${_logging_policy_config}" "find_dependency(fmt CONFIG)" _banned_fmt_config_pos)
  if(NOT _banned_fmt_config_pos EQUAL -1)
    message(FATAL_ERROR "Installed packages must not declare removed fmt dependency: ${_config_template}")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/vcpkg.json" _logging_policy_vcpkg)
if(_logging_policy_vcpkg MATCHES "\"fmt\"")
  message(FATAL_ERROR "vcpkg manifest must not retain a direct fmt dependency")
endif()
if(NOT _logging_policy_vcpkg MATCHES "\"name\"[ \t\r\n]*:[ \t\r\n]*\"spdlog\"[\n\r\t ,{}\"]*\"default-features\"[ \t\r\n]*:[ \t\r\n]*false")
  message(FATAL_ERROR "vcpkg manifest must request spdlog without default fmt features")
endif()

file(READ "${LVR2_SOURCE_DIR}/package.xml" _logging_policy_package_xml)
if(_logging_policy_package_xml MATCHES "libfmt-dev")
  message(FATAL_ERROR "package.xml must not retain libfmt-dev")
endif()

file(READ "${LVR2_SOURCE_DIR}/debian/control" _logging_policy_debian_control)
file(READ "${LVR2_SOURCE_DIR}/cmake/Lvr3Packaging.cmake" _logging_policy_cpack)
foreach(_packaging_variable IN ITEMS _logging_policy_debian_control _logging_policy_cpack)
  if("${${_packaging_variable}}" MATCHES "libfmt-dev")
    message(FATAL_ERROR "Debian/CPack packaging must not retain libfmt-dev")
  endif()
endforeach()

message(STATUS "Format logging policy guard passed")
