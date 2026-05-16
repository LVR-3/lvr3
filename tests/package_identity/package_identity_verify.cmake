# Creates/uses install-prefix fixtures and validates lvr2/lvr3 package identity loading.

set(_SCRIPT_DIR "${CMAKE_CURRENT_LIST_DIR}")
get_filename_component(_PROJECT_DIR "${_SCRIPT_DIR}/../.." ABSOLUTE)

if(DEFINED ENV{TMPDIR} AND NOT "$ENV{TMPDIR}" STREQUAL "")
  set(_TMP_ROOT "$ENV{TMPDIR}")
elseif(DEFINED ENV{TEMP} AND NOT "$ENV{TEMP}" STREQUAL "")
  set(_TMP_ROOT "$ENV{TEMP}")
else()
  set(_TMP_ROOT "${_SCRIPT_DIR}")
endif()

string(RANDOM _RAND)
set(_WORK_ROOT "${_TMP_ROOT}/lvr3-identity-work-${_RAND}")
set(_IDENTITY_CONSUMER_DIR "${_WORK_ROOT}/package_identity_consumer")

function(_copy_lvr_find_modules _lvr2_modules_dir _lvr3_modules_dir)
  set(_LVR_MODULE_SOURCE
    "${_PROJECT_DIR}/cmake/modules/FindFLANN.cmake"
    "${_PROJECT_DIR}/cmake/modules/FindLZ4.cmake"
  )
  foreach(_module IN LISTS _LVR_MODULE_SOURCE)
    file(INSTALL DESTINATION "${_lvr2_modules_dir}" FILES "${_module}")
    file(INSTALL DESTINATION "${_lvr3_modules_dir}" FILES "${_module}")
  endforeach()
endfunction()

function(_write_fake_prefix _prefix _shape)
  set(_LVR2_CONFIG_DIR "${_prefix}/lib/cmake/lvr2")
  set(_LVR3_CONFIG_DIR "${_prefix}/lib/cmake/lvr3")
  set(_LVR2_MODULES "${_LVR2_CONFIG_DIR}/Modules")
  set(_LVR3_MODULES "${_LVR3_CONFIG_DIR}/Modules")
  set(_TL_EXPECTED_CONFIG_DIR "${_prefix}/share/cmake/tl-expected")

  file(REMOVE_RECURSE "${_prefix}")
  file(MAKE_DIRECTORY
    "${_LVR2_CONFIG_DIR}"
    "${_LVR3_CONFIG_DIR}"
    "${_LVR2_MODULES}"
    "${_LVR3_MODULES}"
    "${_TL_EXPECTED_CONFIG_DIR}"
    "${_prefix}/include"
    "${_prefix}/lib"
  )

  _copy_lvr_find_modules("${_LVR2_MODULES}" "${_LVR3_MODULES}")

  file(WRITE "${_prefix}/lib/liblvr2.a" "")
  file(WRITE "${_prefix}/lib/liblvr2${CMAKE_SHARED_LIBRARY_SUFFIX}" "")
  file(WRITE "${_TL_EXPECTED_CONFIG_DIR}/tl-expected-config.cmake"
"if(NOT TARGET tl::expected)\n  add_library(tl::expected INTERFACE IMPORTED)\nendif()\nset(tl-expected_FOUND TRUE)\n")

  if(_shape STREQUAL "static-only")
    set(_TARGETS_CONTENT
"# Fake static-only lvr2 targets for package-identity smoke test
if(NOT TARGET lvr2::lvr2)
  add_library(lvr2::lvr2 STATIC IMPORTED)
  set_target_properties(lvr2::lvr2 PROPERTIES
    IMPORTED_LOCATION \"${_prefix}/lib/liblvr2.a\"
    INTERFACE_LINK_LIBRARIES \"tl::expected\")
endif()
")
  elseif(_shape STREQUAL "dual")
    set(_TARGETS_CONTENT
"# Fake dual static/shared lvr2 targets for package-identity smoke test
if(NOT TARGET lvr2::lvr2)
  add_library(lvr2::lvr2 SHARED IMPORTED)
  set_target_properties(lvr2::lvr2 PROPERTIES
    IMPORTED_LOCATION \"${_prefix}/lib/liblvr2${CMAKE_SHARED_LIBRARY_SUFFIX}\"
    INTERFACE_LINK_LIBRARIES \"tl::expected\")
endif()
if(NOT TARGET lvr2::lvr2_static)
  add_library(lvr2::lvr2_static STATIC IMPORTED)
  set_target_properties(lvr2::lvr2_static PROPERTIES
    IMPORTED_LOCATION \"${_prefix}/lib/liblvr2.a\"
    INTERFACE_LINK_LIBRARIES \"tl::expected\")
endif()
")
  elseif(_shape STREQUAL "shared-only")
    set(_TARGETS_CONTENT
"# Fake shared-only lvr2 targets for package-identity smoke test
if(NOT TARGET lvr2::lvr2)
  add_library(lvr2::lvr2 SHARED IMPORTED)
  set_target_properties(lvr2::lvr2 PROPERTIES
    IMPORTED_LOCATION \"${_prefix}/lib/liblvr2${CMAKE_SHARED_LIBRARY_SUFFIX}\"
    INTERFACE_LINK_LIBRARIES \"tl::expected\")
endif()
")
  else()
    message(FATAL_ERROR "Unknown fake package shape '${_shape}'")
  endif()

  set(_COMPONENT_TARGETS_CONTENT "")
  foreach(_component IN ITEMS geometry io reconstruction texture registration display)
    string(APPEND _COMPONENT_TARGETS_CONTENT
"if(NOT TARGET lvr2::${_component})\n  add_library(lvr2::${_component} INTERFACE IMPORTED)\n  set_target_properties(lvr2::${_component} PROPERTIES\n    INTERFACE_LINK_LIBRARIES \"lvr2::lvr2\")\nendif()\n")
  endforeach()
  string(APPEND _TARGETS_CONTENT "${_COMPONENT_TARGETS_CONTENT}")

  file(WRITE "${_LVR2_CONFIG_DIR}/lvr2-targets.cmake" "${_TARGETS_CONTENT}")

  file(WRITE "${_LVR2_CONFIG_DIR}/lvr2-config-version.cmake"
"set(PACKAGE_VERSION \"25.2.3\")\nset(PACKAGE_VERSION_EXACT TRUE)\nset(PACKAGE_VERSION_COMPATIBLE TRUE)\n")

  file(WRITE "${_LVR2_CONFIG_DIR}/lvr2-config.cmake"
"include(CMakeFindDependencyMacro)
macro(check_required_components _NAME)
  foreach(_component \${\${_NAME}_FIND_COMPONENTS})
    if(NOT \${_NAME}_\${_component}_FOUND)
      if(\${_NAME}_FIND_REQUIRED_\${_component})
        set(\${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()
find_dependency(tl-expected CONFIG)
include(\"${_LVR2_CONFIG_DIR}/lvr2-targets.cmake\")
list(APPEND CMAKE_MODULE_PATH \"${_LVR2_CONFIG_DIR}/Modules\")
set(LVR2_INCLUDE_DIRS \"${_prefix}/include\")
set(LVR2_DEFINITIONS \"\")

set(_LVR2_INSTALL_HAS_STATIC_TARGET FALSE)
if(TARGET lvr2::lvr2_static)
  set(_LVR2_INSTALL_HAS_STATIC_TARGET TRUE)
endif()
if(NOT _LVR2_INSTALL_HAS_STATIC_TARGET AND TARGET lvr2::lvr2)
  get_target_property(_LVR2_AGGREGATE_TYPE lvr2::lvr2 TYPE)
  if(_LVR2_AGGREGATE_TYPE STREQUAL \"STATIC_LIBRARY\")
    if(NOT TARGET lvr2::lvr2_static)
      add_library(lvr2::lvr2_static ALIAS lvr2::lvr2)
    endif()
    set(_LVR2_INSTALL_HAS_STATIC_TARGET TRUE)
  endif()
endif()
option(LVR2_USE_STATIC_LIBS \"Prefer static lvr2 library when available\" \${_LVR2_INSTALL_HAS_STATIC_TARGET})
if(LVR2_USE_STATIC_LIBS AND TARGET lvr2::lvr2_static)
  set(LVR2_LIBRARY lvr2::lvr2_static)
elseif(TARGET lvr2::lvr2)
  set(LVR2_LIBRARY lvr2::lvr2)
else()
  message(FATAL_ERROR \"No installed lvr2 library target is available.\")
endif()
set(LVR2_LIBRARIES \${LVR2_LIBRARY})
set(_LVR2_KNOWN_COMPONENTS geometry io reconstruction texture registration display)
foreach(_LVR2_COMPONENT IN LISTS _LVR2_KNOWN_COMPONENTS)
  if(TARGET lvr2::\${_LVR2_COMPONENT})
    set(lvr2_\${_LVR2_COMPONENT}_FOUND TRUE)
    set(LVR2_\${_LVR2_COMPONENT}_FOUND TRUE)
  else()
    set(lvr2_\${_LVR2_COMPONENT}_FOUND FALSE)
    set(LVR2_\${_LVR2_COMPONENT}_FOUND FALSE)
  endif()
endforeach()
set(_LVR2_MISSING_COMPONENTS)
foreach(_LVR2_COMPONENT IN LISTS lvr2_FIND_COMPONENTS)
  list(FIND _LVR2_KNOWN_COMPONENTS \"\${_LVR2_COMPONENT}\" _LVR2_COMPONENT_INDEX)
  if(_LVR2_COMPONENT_INDEX EQUAL -1)
    set(lvr2_\${_LVR2_COMPONENT}_FOUND FALSE)
    set(LVR2_\${_LVR2_COMPONENT}_FOUND FALSE)
    list(APPEND _LVR2_MISSING_COMPONENTS \${_LVR2_COMPONENT})
  elseif(NOT TARGET lvr2::\${_LVR2_COMPONENT})
    list(APPEND _LVR2_MISSING_COMPONENTS \${_LVR2_COMPONENT})
  endif()
endforeach()
if(_LVR2_MISSING_COMPONENTS)
  list(JOIN _LVR2_MISSING_COMPONENTS \", \" _LVR2_MISSING_COMPONENTS_TEXT)
  set(lvr2_NOT_FOUND_MESSAGE \"lvr2 package does not provide requested component(s): \${_LVR2_MISSING_COMPONENTS_TEXT}. Known components: \${_LVR2_KNOWN_COMPONENTS}\")
endif()
set(lvr2_FOUND TRUE)
check_required_components(lvr2)
set(LVR2_FOUND \${lvr2_FOUND})
")

  file(WRITE "${_LVR3_CONFIG_DIR}/lvr3-config-version.cmake"
"set(PACKAGE_VERSION \"25.2.3\")\nset(PACKAGE_VERSION_EXACT TRUE)\nset(PACKAGE_VERSION_COMPATIBLE TRUE)\n")

  file(READ "${_PROJECT_DIR}/cmake/lvr3-config.cmake.in" _LVR3_CONFIG_TEMPLATE)
  set(_PACKAGE_INIT "set(PACKAGE_PREFIX_DIR \"${_prefix}\")
macro(check_required_components _NAME)
  foreach(_component \${\${_NAME}_FIND_COMPONENTS})
    if(NOT \${_NAME}_\${_component}_FOUND)
      if(\${_NAME}_FIND_REQUIRED_\${_component})
        set(\${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()")
  string(REPLACE "@PACKAGE_INIT@" "${_PACKAGE_INIT}" _LVR3_CONFIG_CONTENT "${_LVR3_CONFIG_TEMPLATE}")
  file(WRITE "${_LVR3_CONFIG_DIR}/lvr3-config.cmake" "${_LVR3_CONFIG_CONTENT}")
endfunction()

set(_ENV_PREFIX "$ENV{LVR2_PACKAGE_IDENTITY_INSTALL_PREFIX}")
set(_ENV_DEPENDENCY_PREFIX_PATH "$ENV{LVR2_PACKAGE_IDENTITY_CMAKE_PREFIX_PATH}")
set(_ENV_TOOLCHAIN_FILE "$ENV{LVR2_PACKAGE_IDENTITY_TOOLCHAIN_FILE}")
set(_PREFIX_REQUESTED FALSE)
set(_PACKAGE_PREFIXES "")
set(_PACKAGE_SHAPES "")

set(_PACKAGE_IDENTITY_CMAKE_ENV_COMMAND "")
if(NOT _ENV_DEPENDENCY_PREFIX_PATH STREQUAL "")
  list(APPEND _PACKAGE_IDENTITY_CMAKE_ENV_COMMAND
    "${CMAKE_COMMAND}" -E env "CMAKE_PREFIX_PATH=${_ENV_DEPENDENCY_PREFIX_PATH}"
  )
endif()

set(_PACKAGE_IDENTITY_CONSUMER_ARGS "")
if(NOT _ENV_TOOLCHAIN_FILE STREQUAL "")
  if(NOT EXISTS "${_ENV_TOOLCHAIN_FILE}")
    message(FATAL_ERROR "LVR2_PACKAGE_IDENTITY_TOOLCHAIN_FILE is set but '${_ENV_TOOLCHAIN_FILE}' does not exist.")
  endif()
  list(APPEND _PACKAGE_IDENTITY_CONSUMER_ARGS
    "-DCMAKE_TOOLCHAIN_FILE=${_ENV_TOOLCHAIN_FILE}"
  )
endif()

if(DEFINED ENV{LVR2_PACKAGE_IDENTITY_INSTALL_PREFIX} AND NOT _ENV_PREFIX STREQUAL "")
  set(_PREFIX_REQUESTED TRUE)
  if(NOT IS_DIRECTORY "${_ENV_PREFIX}")
    message(FATAL_ERROR "LVR2_PACKAGE_IDENTITY_INSTALL_PREFIX is set but '${_ENV_PREFIX}' does not exist.")
  endif()
  list(APPEND _PACKAGE_PREFIXES "${_ENV_PREFIX}")
  list(APPEND _PACKAGE_SHAPES "requested")
else()
  foreach(_shape IN ITEMS static-only dual shared-only)
    set(_prefix "${_TMP_ROOT}/lvr3-identity-${_shape}-${_RAND}")
    _write_fake_prefix("${_prefix}" "${_shape}")
    list(APPEND _PACKAGE_PREFIXES "${_prefix}")
    list(APPEND _PACKAGE_SHAPES "${_shape}")
  endforeach()
endif()

# Write a tiny consumer project used by all cases below. Keep this outside the
# install prefix so explicit installed prefixes can be read-only.
file(REMOVE_RECURSE "${_WORK_ROOT}")
file(MAKE_DIRECTORY "${_IDENTITY_CONSUMER_DIR}")
file(READ
  "${_SCRIPT_DIR}/package_identity_consumer.cmake"
  _PACKAGE_IDENTITY_CONSUMER_CMAKELISTS
)
file(WRITE
  "${_IDENTITY_CONSUMER_DIR}/CMakeLists.txt"
  "${_PACKAGE_IDENTITY_CONSUMER_CMAKELISTS}"
)

set(_CASES
  "legacy"
  "modern"
  "legacy_then_modern"
  "modern_then_legacy"
  "legacy_unknown_quiet"
  "modern_unknown_quiet"
  "legacy_unknown_optional"
  "modern_unknown_optional"
)
set(_EXPECTED_FAILURE_CASES
  "legacy_unknown_required"
  "modern_unknown_required"
)

list(LENGTH _PACKAGE_PREFIXES _PREFIX_COUNT)
math(EXPR _LAST_PREFIX_INDEX "${_PREFIX_COUNT} - 1")
foreach(_index RANGE 0 ${_LAST_PREFIX_INDEX})
  list(GET _PACKAGE_PREFIXES ${_index} _prefix)
  list(GET _PACKAGE_SHAPES ${_index} _shape)

  foreach(_case IN LISTS _CASES)
    execute_process(
      COMMAND ${_PACKAGE_IDENTITY_CMAKE_ENV_COMMAND} "${CMAKE_COMMAND}"
        -S "${_IDENTITY_CONSUMER_DIR}"
        -B "${_IDENTITY_CONSUMER_DIR}/build-${_shape}-${_case}"
        -DCHECK_PREFIX=${_prefix}
        -DCHECK_ORDER=${_case}
        -DCHECK_SHAPE=${_shape}
        ${_PACKAGE_IDENTITY_CONSUMER_ARGS}
      RESULT_VARIABLE _rv
      OUTPUT_VARIABLE _out
      ERROR_VARIABLE _err
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
    )

    if(NOT _rv EQUAL 0)
      message(FATAL_ERROR "Package identity smoke check '${_shape}/${_case}' failed.\n${_out}\n${_err}")
    endif()
  endforeach()

  foreach(_case IN LISTS _EXPECTED_FAILURE_CASES)
    execute_process(
      COMMAND ${_PACKAGE_IDENTITY_CMAKE_ENV_COMMAND} "${CMAKE_COMMAND}"
        -S "${_IDENTITY_CONSUMER_DIR}"
        -B "${_IDENTITY_CONSUMER_DIR}/build-${_shape}-${_case}"
        -DCHECK_PREFIX=${_prefix}
        -DCHECK_ORDER=${_case}
        -DCHECK_SHAPE=${_shape}
        ${_PACKAGE_IDENTITY_CONSUMER_ARGS}
      RESULT_VARIABLE _rv
      OUTPUT_VARIABLE _out
      ERROR_VARIABLE _err
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
    )

    if(_rv EQUAL 0)
      message(FATAL_ERROR "Package identity smoke check '${_shape}/${_case}' unexpectedly passed.")
    endif()
  endforeach()
endforeach()

message(STATUS "Package identity helper checks passed for prefixes: ${_PACKAGE_PREFIXES}")

file(REMOVE_RECURSE "${_WORK_ROOT}")
if(NOT _PREFIX_REQUESTED)
  foreach(_prefix IN LISTS _PACKAGE_PREFIXES)
    file(REMOVE_RECURSE "${_prefix}")
  endforeach()
endif()
