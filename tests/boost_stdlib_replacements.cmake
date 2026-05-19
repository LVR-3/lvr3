if(NOT DEFINED LVR2_SOURCE_DIR)
  message(FATAL_ERROR "LVR2_SOURCE_DIR is required")
endif()

set(_roots
  include
  src
  examples
  tests
  cmake
  vcpkg.json
  debian
  package.xml
  .github
  .gitlab-ci.yml
)

set(_banned_code_patterns
  "#include[ \t]*[<\"]boost/(filesystem|optional|shared_array|shared_ptr|smart_ptr|variant|type_index|core/typeinfo|format|foreach|thread|timer|system|algorithm)"
  "boost::(filesystem|optional|none|shared_array|shared_ptr|make_shared|static_pointer_cast|variant|get|apply_visitor|static_visitor|typeindex|format|thread|mutex|timer|system|algorithm|split|is_any_of|to_upper_copy|core)"
  "BOOST_(FOREACH|CORE)"
  "make_shared_array"
  "std::optional<[^>;]+&"
  "getAttribute<[^;\r\n]+\\.get\\("
)

set(_banned_metadata_patterns
  "boost-(filesystem|thread|timer)"
  "libboost-(filesystem|thread|timer|system|test)-dev"
  "Boost_(FILESYSTEM|THREAD)_LIBRARY"
)

foreach(_root IN LISTS _roots)
  set(_path "${LVR2_SOURCE_DIR}/${_root}")
  if(IS_DIRECTORY "${_path}")
    file(GLOB_RECURSE _files LIST_DIRECTORIES false "${_path}/*")
  elseif(EXISTS "${_path}")
    set(_files "${_path}")
  else()
    set(_files "")
  endif()

  foreach(_file IN LISTS _files)
    if(IS_DIRECTORY "${_file}")
      continue()
    endif()
    file(RELATIVE_PATH _rel "${LVR2_SOURCE_DIR}" "${_file}")
    if(_rel MATCHES "(^|/)docs/|(^|/)tests/boost_retirement_inventory\\.cmake$|(^|/)tests/no_boost_cli_parser_dependency\\.cmake$|(^|/)tests/boost_stdlib_replacements\\.cmake$")
      continue()
    endif()

    file(READ "${_file}" _content)
    if(_rel MATCHES "^(include|src|examples|tests)/")
      foreach(_pattern IN LISTS _banned_code_patterns)
        if(_content MATCHES "${_pattern}")
          message(FATAL_ERROR "Boost standard-library replacement guard failed in ${_rel}: ${_pattern}")
        endif()
      endforeach()
    endif()

    if(_rel MATCHES "^(cmake/|vcpkg\\.json$|debian/|package\\.xml$|\\.github/|\\.gitlab-ci\\.yml$)")
      foreach(_pattern IN LISTS _banned_metadata_patterns)
        if(_content MATCHES "${_pattern}")
          message(FATAL_ERROR "Boost standard dependency metadata remains in ${_rel}: ${_pattern}")
        endif()
      endforeach()
    endif()
  endforeach()
endforeach()

set(_required_tokens
  "std::filesystem"
  "std::optional"
  "std::variant"
  "std::shared_ptr<T[]>"
  "std::span"
  "std::chrono"
)

file(READ "${LVR2_SOURCE_DIR}/docs/dependencies/boost-removal-inventory.md" _inventory)
foreach(_token IN LISTS _required_tokens)
  string(FIND "${_inventory}" "${_token}" _found)
  if(_found EQUAL -1)
    message(FATAL_ERROR "Boost inventory missing stdlib replacement token: ${_token}")
  endif()
endforeach()
