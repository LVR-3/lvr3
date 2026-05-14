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
    "${_PROJECT_DIR}/CMakeModules/FindFLANN.cmake"
    "${_PROJECT_DIR}/CMakeModules/FindLZ4.cmake"
    "${_PROJECT_DIR}/CMakeModules/FindOpenNI.cmake"
    "${_PROJECT_DIR}/CMakeModules/FindOpenNI2.cmake"
    "${_PROJECT_DIR}/CMakeModules/FindQVTK.cmake"
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

  file(REMOVE_RECURSE "${_prefix}")
  file(MAKE_DIRECTORY
    "${_LVR2_CONFIG_DIR}"
    "${_LVR3_CONFIG_DIR}"
    "${_LVR2_MODULES}"
    "${_LVR3_MODULES}"
    "${_prefix}/include"
    "${_prefix}/lib"
  )

  _copy_lvr_find_modules("${_LVR2_MODULES}" "${_LVR3_MODULES}")

  file(WRITE "${_prefix}/lib/liblvr2.a" "")
  file(WRITE "${_prefix}/lib/liblvr2${CMAKE_SHARED_LIBRARY_SUFFIX}" "")

  if(_shape STREQUAL "static-only")
    set(_TARGETS_CONTENT
"# Fake static-only lvr2 targets for package-identity smoke test
if(NOT TARGET lvr2::lvr2)
  add_library(lvr2::lvr2 STATIC IMPORTED)
  set_target_properties(lvr2::lvr2 PROPERTIES IMPORTED_LOCATION \"${_prefix}/lib/liblvr2.a\")
endif()
")
  elseif(_shape STREQUAL "dual")
    set(_TARGETS_CONTENT
"# Fake dual static/shared lvr2 targets for package-identity smoke test
if(NOT TARGET lvr2::lvr2)
  add_library(lvr2::lvr2 SHARED IMPORTED)
  set_target_properties(lvr2::lvr2 PROPERTIES IMPORTED_LOCATION \"${_prefix}/lib/liblvr2${CMAKE_SHARED_LIBRARY_SUFFIX}\")
endif()
if(NOT TARGET lvr2::lvr2_static)
  add_library(lvr2::lvr2_static STATIC IMPORTED)
  set_target_properties(lvr2::lvr2_static PROPERTIES IMPORTED_LOCATION \"${_prefix}/lib/liblvr2.a\")
endif()
")
  elseif(_shape STREQUAL "shared-only")
    set(_TARGETS_CONTENT
"# Fake shared-only lvr2 targets for package-identity smoke test
if(NOT TARGET lvr2::lvr2)
  add_library(lvr2::lvr2 SHARED IMPORTED)
  set_target_properties(lvr2::lvr2 PROPERTIES IMPORTED_LOCATION \"${_prefix}/lib/liblvr2${CMAKE_SHARED_LIBRARY_SUFFIX}\")
endif()
")
  else()
    message(FATAL_ERROR "Unknown fake package shape '${_shape}'")
  endif()

  file(WRITE "${_LVR2_CONFIG_DIR}/lvr2-targets.cmake" "${_TARGETS_CONTENT}")

  file(WRITE "${_LVR2_CONFIG_DIR}/lvr2-config-version.cmake"
"set(PACKAGE_VERSION \"25.2.3\")\nset(PACKAGE_VERSION_EXACT TRUE)\nset(PACKAGE_VERSION_COMPATIBLE TRUE)\n")

  file(WRITE "${_LVR2_CONFIG_DIR}/lvr2-config.cmake"
"include(\"${_LVR2_CONFIG_DIR}/lvr2-targets.cmake\")
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
set(lvr2_FOUND TRUE)
set(LVR2_FOUND TRUE)
")

  file(WRITE "${_LVR3_CONFIG_DIR}/lvr3-config-version.cmake"
"set(PACKAGE_VERSION \"25.2.3\")\nset(PACKAGE_VERSION_EXACT TRUE)\nset(PACKAGE_VERSION_COMPATIBLE TRUE)\n")

  file(READ "${_PROJECT_DIR}/CMakeModules/lvr3-config.cmake.in" _LVR3_CONFIG_TEMPLATE)
  set(_PACKAGE_INIT "set(PACKAGE_PREFIX_DIR \"${_prefix}\")")
  string(REPLACE "@PACKAGE_INIT@" "${_PACKAGE_INIT}" _LVR3_CONFIG_CONTENT "${_LVR3_CONFIG_TEMPLATE}")
  file(WRITE "${_LVR3_CONFIG_DIR}/lvr3-config.cmake" "${_LVR3_CONFIG_CONTENT}")
endfunction()

set(_ENV_PREFIX "$ENV{LVR2_PACKAGE_IDENTITY_INSTALL_PREFIX}")
set(_PREFIX_REQUESTED FALSE)
set(_PACKAGE_PREFIXES "")
set(_PACKAGE_SHAPES "")

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
)

list(LENGTH _PACKAGE_PREFIXES _PREFIX_COUNT)
math(EXPR _LAST_PREFIX_INDEX "${_PREFIX_COUNT} - 1")
foreach(_index RANGE 0 ${_LAST_PREFIX_INDEX})
  list(GET _PACKAGE_PREFIXES ${_index} _prefix)
  list(GET _PACKAGE_SHAPES ${_index} _shape)

  foreach(_case IN LISTS _CASES)
    execute_process(
      COMMAND "${CMAKE_COMMAND}"
        -S "${_IDENTITY_CONSUMER_DIR}"
        -B "${_IDENTITY_CONSUMER_DIR}/build-${_shape}-${_case}"
        -DCHECK_PREFIX=${_prefix}
        -DCHECK_ORDER=${_case}
        -DCHECK_SHAPE=${_shape}
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
endforeach()

message(STATUS "Package identity helper checks passed for prefixes: ${_PACKAGE_PREFIXES}")

file(REMOVE_RECURSE "${_WORK_ROOT}")
if(NOT _PREFIX_REQUESTED)
  foreach(_prefix IN LISTS _PACKAGE_PREFIXES)
    file(REMOVE_RECURSE "${_prefix}")
  endforeach()
endif()
