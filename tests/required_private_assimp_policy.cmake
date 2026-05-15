if(NOT DEFINED LVR2_SOURCE_DIR OR LVR2_SOURCE_DIR STREQUAL "")
  message(FATAL_ERROR "LVR2_SOURCE_DIR must be provided")
endif()

file(READ "${LVR2_SOURCE_DIR}/CMakeLists.txt" _TOP_CMAKE)
file(READ "${LVR2_SOURCE_DIR}/CMakePresets.json" _PRESETS_JSON)
file(READ "${LVR2_SOURCE_DIR}/vcpkg.json" _VCPKG_JSON)

if(_TOP_CMAKE MATCHES [[option\(LVR2_WITH_ASSIMP]])
  message(FATAL_ERROR "Assimp must not be exposed as a public LVR2_WITH_ASSIMP build option")
endif()

if(_TOP_CMAKE MATCHES "LVR2_USE_SYSTEM_ASSIMP" OR _PRESETS_JSON MATCHES "LVR2_USE_SYSTEM_ASSIMP")
  message(FATAL_ERROR "Assimp must not be exposed in the public per-package system opt-out option vocabulary")
endif()

string(FIND "${_TOP_CMAKE}" "lvr2_find_package(assimp CONFIG QUIET)" _ASSIMP_FIND)
if(_ASSIMP_FIND EQUAL -1)
  message(FATAL_ERROR "CMake must require the private Assimp package internally for mesh asset I/O")
endif()

if(NOT _TOP_CMAKE MATCHES "Required private mesh asset I/O is shared-only")
  message(FATAL_ERROR "CMake must keep a clear privacy-first shared-only guard for required private mesh I/O")
endif()

string(FIND "${_TOP_CMAKE}" "find_dependency(assimp" _ASSIMP_DEP_LOWER)
string(FIND "${_TOP_CMAKE}" "find_dependency(Assimp" _ASSIMP_DEP_UPPER)
if(NOT _ASSIMP_DEP_LOWER EQUAL -1 OR NOT _ASSIMP_DEP_UPPER EQUAL -1)
  message(FATAL_ERROR "Top-level CMake must not add public find_dependency(assimp)")
endif()

if(NOT _VCPKG_JSON MATCHES "\"assimp\"")
  message(FATAL_ERROR "vcpkg manifest must include assimp as a default dependency")
endif()

if(_VCPKG_JSON MATCHES [["assimp"[ 	
]*:]])
  message(FATAL_ERROR "assimp must not remain a feature-only dependency")
endif()

message(STATUS "Required/private Assimp policy is encoded without public option or package dependency leakage")
