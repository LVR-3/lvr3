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
  "fmt::streamed[ \t\r\n]*\\("
  "#[ \t]*include[ \t]*[<\"]fmt/ostream\\.h[>\"]"
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
          "Removed stream/timestamp logging pattern remains in ${_logging_policy_rel}: ${_logging_policy_pattern}")
      endif()
    endforeach()
  endforeach()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/include/lvr2/util/Logging.hpp" _logging_policy_header)
foreach(_required_token IN ITEMS
  "namespace log"
  "SourceLocation"
  "logger_handle"
  "fmt::runtime"
  "void info"
  "void warning"
  "void error"
  "write_runtime"
)
  if(NOT _logging_policy_header MATCHES "${_required_token}")
    message(FATAL_ERROR "Logging facade header is missing required token '${_required_token}'")
  endif()
endforeach()

foreach(_banned_header_token IN ITEMS
  "fmt::format_string"
  "fmt::format[ \\t\\r\\n]*\\("
)
  if(_logging_policy_header MATCHES "${_banned_header_token}")
    message(FATAL_ERROR "Logging facade must not pre-format or expose fmt format-string types: ${_banned_header_token}")
  endif()
endforeach()

foreach(_public_signature_leak IN ITEMS
  "void[ \\t\\r\\n]+(trace|debug|info|warning|warn|error|write)[^;{\\n]*(fmt::|spdlog::)"
)
  if(_logging_policy_header MATCHES "${_public_signature_leak}")
    message(FATAL_ERROR "Stable public logging signatures must not mention fmt:: or spdlog::")
  endif()
endforeach()

file(READ "${LVR2_SOURCE_DIR}/migration_guide.md" _logging_policy_migration_guide)
if(_logging_policy_migration_guide MATCHES "fmt::streamed")
  message(FATAL_ERROR "Public logging migration examples must not recommend fmt::streamed")
endif()

file(READ "${LVR2_SOURCE_DIR}/cmake/lvr2-config.cmake.in" _logging_policy_lvr2_config)
if(NOT _logging_policy_lvr2_config MATCHES "find_dependency\\(spdlog CONFIG\\)")
  message(FATAL_ERROR "Installed packages must declare spdlog for the inline logging detail layer")
endif()

message(STATUS "Format logging policy guard passed")
