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
  "fmt::format_string"
  "void info"
  "void warning"
  "void error"
  "write_runtime"
)
  if(NOT _logging_policy_header MATCHES "${_required_token}")
    message(FATAL_ERROR "Logging facade header is missing required token '${_required_token}'")
  endif()
endforeach()

if(_logging_policy_header MATCHES "spdlog")
  message(FATAL_ERROR "Public logging header must not expose the private spdlog sink")
endif()

file(READ "${LVR2_SOURCE_DIR}/cmake/lvr2-config.cmake.in" _logging_policy_lvr2_config)
if(_logging_policy_lvr2_config MATCHES "find_dependency\\(spdlog CONFIG\\)" AND
   NOT _logging_policy_lvr2_config MATCHES "_LVR2_PACKAGE_HAS_STATIC_TARGET")
  message(FATAL_ERROR "spdlog may appear in installed config only as a static LINK_ONLY closure dependency")
endif()

message(STATUS "Format logging policy guard passed")
