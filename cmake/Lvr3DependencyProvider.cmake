# Dependency acquisition policy
#  - vcpkg manifest/toolchain is the default dependency path
#  - system package escapes are explicit and per-package via LVR2_USE_SYSTEM_<PKG>
set(_LVR2_DEFAULT_VCPKG_TOOLCHAIN_FILE "")
if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
  set(_LVR2_DEFAULT_VCPKG_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
  set(_LVR2_DEFAULT_VCPKG_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

option(LVR2_WITH_VCPKG "Use vcpkg manifest/toolchain dependency lookup" ON)
set(LVR2_VCPKG_TOOLCHAIN_FILE
    "${_LVR2_DEFAULT_VCPKG_TOOLCHAIN_FILE}"
    CACHE FILEPATH "Path to vcpkg toolchain file")
if(NOT DEFINED LVR2_IGNORE_SYSTEM_PACKAGES)
  set(LVR2_IGNORE_SYSTEM_PACKAGES ${LVR2_WITH_VCPKG}
      CACHE BOOL "Ignore CMake system/env package search paths unless a package-specific system opt-out is enabled")
endif()

if(CMAKE_TOOLCHAIN_FILE)
  message(STATUS "Using user-supplied CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
elseif(LVR2_WITH_VCPKG)
  if(EXISTS "${LVR2_VCPKG_TOOLCHAIN_FILE}")
    set(CMAKE_TOOLCHAIN_FILE "${LVR2_VCPKG_TOOLCHAIN_FILE}"
        CACHE FILEPATH "vcpkg toolchain file" FORCE)
  else()
    message(FATAL_ERROR
      "vcpkg-first dependency policy makes vcpkg the primary LVR dependency path. No vcpkg toolchain was found.\n"
      "Set VCPKG_ROOT, set -DCMAKE_TOOLCHAIN_FILE=<path>/scripts/buildsystems/vcpkg.cmake, "
      "or set -DLVR2_VCPKG_TOOLCHAIN_FILE=<path>.\n"
      "For a system-package escape hatch, configure -DLVR2_WITH_VCPKG=OFF and enable "
      "only the needed -DLVR2_USE_SYSTEM_<PKG>=ON options documented in migration_guide.md.")
  endif()
endif()

if(LVR2_IGNORE_SYSTEM_PACKAGES)
  message(STATUS "LVR2_IGNORE_SYSTEM_PACKAGES=ON: disabling CMake system/env search by default")
  set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH OFF)
  set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)
endif()

set(_LVR2_SYSTEM_PACKAGE_OPTOUTS
  tl-expected
  TBB
  TIFF
  GDAL
  OpenCV
  FLANN
  LZ4
  GSL
  Eigen3
  HDF5
  OpenGL
  GLUT
  yaml-cpp
  spdlog
  HighFive
  rply
  LASlib
  embree
  MPI
  CUDAToolkit
  CUDA
  RDB
  OpenCL
  PCL
  OpenMP
  RiVLib
  draco
  Doxygen
  GTest
  Python3
  PkgConfig
)
foreach(_LVR2_PACKAGE IN LISTS _LVR2_SYSTEM_PACKAGE_OPTOUTS)
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" _LVR2_PACKAGE_OPTION "${_LVR2_PACKAGE}")
  string(TOUPPER "${_LVR2_PACKAGE_OPTION}" _LVR2_PACKAGE_OPTION)
  option(LVR2_USE_SYSTEM_${_LVR2_PACKAGE_OPTION}
         "Use a system-installed ${_LVR2_PACKAGE} package instead of the vcpkg-provided package"
         OFF)
endforeach()
