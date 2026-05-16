#------------------------------------------------------------------------------
# Searching for Embree: needs to be placed above "set(CMAKE_MODULE_PATH)".
#------------------------------------------------------------------------------
lvr2_find_package(embree 4 QUIET)
if(embree_FOUND)
  message(STATUS "Found Embree")
  include_directories(${EMBREE_INCLUDE_DIRS})
  list(APPEND LVR2_DEFINITIONS -DLVR2_USE_EMBREE)
  list(APPEND LVR2_DEFINITIONS -DLVR2_EMBREE_VERSION=4)
  set(LVR2_EMBREE_VERSION 4)
else(embree_FOUND)
  lvr2_find_package(embree 3 QUIET)
  if(embree_FOUND)
    message(STATUS "Found Embree")
    include_directories(${EMBREE_INCLUDE_DIRS})
    list(APPEND LVR2_DEFINITIONS -DLVR2_USE_EMBREE)
    list(APPEND LVR2_DEFINITIONS -DLVR2_EMBREE_VERSION=3)
    set(LVR2_EMBREE_VERSION 3)
  endif(embree_FOUND)
endif(embree_FOUND)

set(CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake/modules
  ${CMAKE_MODULE_PATH}
)

message(STATUS ${CMAKE_MODULE_PATH})

set(LVR2_MESH_IO_ASSIMP_AVAILABLE OFF)
set(LVR2_ASSIMP_TARGET "")
set(LVR2_ASSIMP_LIBRARIES "")
set(LVR2_ASSIMP_INCLUDE_DIRS "")
lvr2_find_package(assimp CONFIG QUIET)
if(TARGET assimp::assimp)
  set(LVR2_MESH_IO_ASSIMP_AVAILABLE ON)
  set(LVR2_ASSIMP_TARGET assimp::assimp)
elseif(assimp_FOUND AND ASSIMP_LIBRARIES)
  set(LVR2_MESH_IO_ASSIMP_AVAILABLE ON)
  set(LVR2_ASSIMP_LIBRARIES ${ASSIMP_LIBRARIES})
  set(LVR2_ASSIMP_INCLUDE_DIRS ${ASSIMP_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "Required private mesh asset I/O needs an assimp CONFIG package with assimp::assimp or ASSIMP_LIBRARIES; no vendored/download fallback is provided.")
endif()
message(STATUS "Required private mesh asset I/O backend enabled for shared lvr2 target only")

include_directories(include)
# Build-only include root for private implementation headers removed from the
# installed public API. Do not add this path to install/export interfaces.
include_directories(src/liblvr2/include)

###############################################################################
# Compiler specific settings
###############################################################################

# Add -O0 to remove optimizations when using gcc
if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g")
endif(CMAKE_COMPILER_IS_GNUCC)

if(MSVC)
  list(APPEND LVR2_DEFINITIONS -DNOMINMAX)
    set(HAVE_CXX_ATOMICS64_WITHOUT_LIB True)
    set(HAVE_CXX_ATOMICS_WITHOUT_LIB True)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
  add_compile_definitions(_HAS_STD_BYTE=0)
  add_compile_definitions(__TBB_NO_IMPLICIT_LINKAGE)
  #add_compile_definitions(__TBBMALLOC_NO_IMPLICIT_LINKAGE=1)
  add_compile_definitions(_USE_MATH_DEFINES)
else(MSVC)
  add_compile_options(-fmessage-length=0 -fPIC -Wno-deprecated)
endif(MSVC)

lvr2_find_package(MPI)
if(MPI_FOUND)
    message(STATUS "Found MPI")
    include_directories(${MPI_CXX_INCLUDE_PATH})
endif(MPI_FOUND)

###############################################################################
# REQUIRED DEPENDENCIES
###############################################################################

#------------------------------------------------------------------------------
# Searching for tl-expected (public result vocabulary dependency)
#------------------------------------------------------------------------------
lvr2_find_package(tl-expected CONFIG REQUIRED)

#------------------------------------------------------------------------------
# Searching for Thread Building Blocks library
#------------------------------------------------------------------------------
lvr2_find_package(TBB REQUIRED)
if(TBB_FOUND)
    message(STATUS "Found TBB library")
endif(TBB_FOUND)

#------------------------------------------------------------------------------
# Searching for TIFF
#------------------------------------------------------------------------------
lvr2_find_package(TIFF REQUIRED)
include_directories(${TIFF_INCLUDE_DIRS})

#------------------------------------------------------------------------------
# Searching for GDAL
#------------------------------------------------------------------------------
lvr2_find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIR})

#------------------------------------------------------------------------------
# Searching for OpenCV
#------------------------------------------------------------------------------
set(OPEN_CV_COMPONENTS "core;imgproc;imgcodecs;features2d;calib3d")
lvr2_find_package(OpenCV 3 QUIET COMPONENTS ${OPEN_CV_COMPONENTS})
if (NOT OpenCV_FOUND)
  lvr2_find_package(OpenCV 4 COMPONENTS ${OPEN_CV_COMPONENTS})
  if(OpenCV_FOUND)
	message(STATUS "Found OpenCV 4")
  endif()
else()
  message(STATUS "Found OpenCV 3")
endif()
include_directories( ${OpenCV_INCLUDE_DIRS} )

if(LVR2_WITH_CV_NONFREE)
  message(STATUS "Using OpenCV non-free module")
  list(APPEND LVR2_DEFINITIONS -DLVR2_USE_CV_NONFREE)
endif(LVR2_WITH_CV_NONFREE)
#------------------------------------------------------------------------------
# Searching for FLANN
#------------------------------------------------------------------------------
lvr2_find_package(FLANN REQUIRED)
message(STATUS "Found FLANN library: ${FLANN_INCLUDE_DIR}")
include_directories(${FLANN_INCLUDE_DIR})

#------------------------------------------------------------------------------
# Searching for LZ4
#------------------------------------------------------------------------------
lvr2_find_package(LZ4 REQUIRED)
include_directories(${LZ4_INCLUDE_DIR})
message(STATUS "Found LZ4 library: ${LZ4_INCLUDE_DIR}")

#------------------------------------------------------------------------------
# Searching for GSL
#------------------------------------------------------------------------------
lvr2_find_package(GSL REQUIRED)
include_directories(${GSL_INCLUDE_DIRS})
message(STATUS "Found GSL")

#------------------------------------------------------------------------------
# Searching for Eigen3
#------------------------------------------------------------------------------
lvr2_find_package(Eigen3 REQUIRED)
# include_directories(${EIGEN3_INCLUDE_DIR})
# message(STATUS "Found Eigen3: ${EIGEN3_INCLUDE_DIR}")

#------------------------------------------------------------------------------
# Searching for Boost
#------------------------------------------------------------------------------
#set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_COMPONENTS
  program_options
  filesystem
  thread
  serialization
  timer
  iostreams
  date_time)

if(MPI_FOUND)
  set(Boost_COMPONENTS ${Boost_COMPONENTS} mpi)
  message(STATUS "Found MPI. Adding Boost MPI library to required components.")
endif()

lvr2_find_package(Boost COMPONENTS ${Boost_COMPONENTS} CONFIG REQUIRED QUIET)
message(STATUS "Found Boost libraries")

link_directories(${Boost_LIBRARY_DIR})
include_directories(${Boost_INCLUDE_DIRS})
add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})

#------------------------------------------------------------------------------
# Searching for HDF5
#------------------------------------------------------------------------------
lvr2_find_package(HDF5 REQUIRED COMPONENTS C CXX HL)
include_directories(${HDF5_INCLUDE_DIRS})
message(STATUS "Found HDF5")

#------------------------------------------------------------------------------
# Searching for OpenGL
#------------------------------------------------------------------------------
lvr2_find_package(OpenGL COMPONENTS OpenGL)
include_directories(${OPENGL_INCLUDE_DIR})
message(STATUS "Found OpenGL: ${OPENGL_INCLUDE_DIR}")

if(APPLE)
  include_directories(/System/Library/Frameworks/GLUT.framework/Headers)
  include_directories(/System/Library/Frameworks/OpenGL.framework/Headers)
  IF(EXISTS "/opt/local/lib")
    link_directories(/opt/local/lib)
  endif()
endif(APPLE)

#------------------------------------------------------------------------------
## Searching for glut
#------------------------------------------------------------------------------
lvr2_find_package(GLUT)
if(GLUT_FOUND)
  message(STATUS "Found OpenGL Utility Toolkit via cmake: ${GLUT_INCLUDE_DIRS}" )
  include_directories(${GLUT_INCLUDE_DIRS})
else(GLUT_FOUND)
  lvr2_find_package(PkgConfig REQUIRED)
  pkg_check_modules(GLUT glut freeglut FREEGLUT REQUIRED)
  message(STATUS "Found OpenGL Utility Toolkit via pkg_config: ${GLUT_LIBDIR} ${FREEGLUT_LIBDIR}" )
  include_directories(${GLUT_INCLUDEDIR})
  link_directories(${GLUT_LIBDIR})
  set(GLUT_LIBRARIES ${pkgcfg_lib_GLUT_glut})
  message(STATUS "Using ugly cmake cache hack to provide GLUT libraries: " : ${GLUT_LIBRARIES})
endif(GLUT_FOUND)

###############################################################################
# OPTIONAL DEPENDENCIES
###############################################################################

#------------------------------------------------------------------------------
# Searching for CUDA
#------------------------------------------------------------------------------
if(MSVC OR APPLE)
    set(LVR2_WITH_CUDA OFF CACHE BOOL "" FORCE)
endif(MSVC OR APPLE)

if(LVR2_WITH_CUDA)

  cmake_policy(SET CMP0104 NEW)

  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES native)
  endif()

    # default flags are not set when including cuda
  if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)

    if(NOT CUDA_NVCC_FLAGS_DEBUG)
      set(CUDA_NVCC_FLAGS_DEBUG "-g" CACHE STRING "" FORCE)
    endif()

    if(NOT CUDA_NVCC_FLAGS_MINSIZEREL)
      set(CUDA_NVCC_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE STRING "" FORCE)
    endif()

    if(NOT CUDA_NVCC_FLAGS_RELEASE)
      set(CUDA_NVCC_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "" FORCE)
    endif()

    if(NOT CUDA_NVCC_FLAGS_RELWITHDEBINFO)
      set(CUDA_NVCC_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE STRING "" FORCE)
    endif()

  endif()

  # set(CUDA_STANDARD 14)

  include(CheckLanguage)
  check_language(CUDA)
  if(CMAKE_CUDA_COMPILER)
      lvr2_find_package(CUDAToolkit QUIET)
      if(CUDAToolkit_FOUND)
          enable_language(CUDA)
          set(CUDA_FOUND True)
          # for backwards compatibility
          set(CUDA_VERSION ${CUDAToolkit_VERSION})
          set(CUDA_INCLUDE_DIRS "") # is in target instead
          set(CUDA_LIBRARIES CUDA::cudart)
          set(CUDA_cusolver_LIBRARY CUDA::cusolver)
          set(CUDA_cublas_LIBRARY CUDA::cublas)
          set(CUDA_DRIVER_LIBRARY CUDA::cuda_driver)
          set(CUDA_CUDA_LIBRARY CUDA::cuda_driver)
          set(CUDA_NVRTC_LIBRARY CUDA::nvrtc)
      else()
          lvr2_find_package(CUDA)
          if(CUDA_FOUND)
              enable_language(CUDA)
              set(CUDA_DRIVER_LIBRARY cuda)
          else()
              message(STATUS "Neither CudaToolkit nor CUDA found!")
          endif(CUDA_FOUND)
      endif(CUDAToolkit_FOUND)
  endif(CMAKE_CUDA_COMPILER)

  if(CUDA_FOUND)
    message(STATUS "Found CUDA")
    include_directories(${CUDA_INCLUDE_DIRS})
    list(APPEND LVR2_DEFINITIONS -DLVR2_USE_CUDA)
  endif(CUDA_FOUND)

endif(LVR2_WITH_CUDA)

#------------------------------------------------------------------------------
# Searching for RDB
#------------------------------------------------------------------------------
lvr2_find_package(RDB)
if(RDB_FOUND)
    include_directories(${RDB_INCLUDE_DIRS})
    message(STATUS "Found LibRDB at ${RDB_INCLUDE_DIRS}, ${RDB_LIBRARIES}.")

    list(APPEND LVR2_DEFINITIONS -DLVR2_USE_RDB)

    install(TARGETS rdbcpp
            EXPORT lvr2-targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )
endif(RDB_FOUND)

#------------------------------------------------------------------------------
# Searching for OpenCl
#------------------------------------------------------------------------------
if(LVR2_WITH_OPENCL)
  lvr2_find_package(OpenCL2)
  if(OPENCL_FOUND)
    message(STATUS "Found OpenCL")
      include_directories(${OPENCL_INCLUDE_DIR})
      list(APPEND LVR2_DEFINITIONS -DLVR2_USE_OPENCL)
      if(OpenCL_NEW_API)
        list(APPEND LVR2_DEFINITIONS -DLVR2_USE_OPENCL_NEW_API)
      endif(OpenCL_NEW_API)
  endif(OPENCL_FOUND)
endif(LVR2_WITH_OPENCL)

#------------------------------------------------------------------------------
## Searching for PCL
#------------------------------------------------------------------------------
if(LVR2_WITH_PCL)
    lvr2_find_package(PCL)
    if(PCL_FOUND)
        # PCL only results in linker error due to internal mpi dependency
        lvr2_find_package(MPI)
        if(MPI_FOUND)
            message(STATUS "PCL and MPI found. Compile with PCL support.")
            include_directories(${PCL_INCLUDE_DIRS})
            link_directories(${PCL_LIBRARY_DIRS})
            add_definitions(${PCL_DEFINITIONS})
            list(APPEND LVR2_DEFINITIONS -DLVR2_USE_PCL)
        else(MPI_FOUND)
            message(WARNING "PCL found but no mpi. To use PCL support please install MPI")
            unset(PCL_FOUND)
        endif(MPI_FOUND)
    else(PCL_FOUND)
        message(STATUS "-- No PCL found." )
        message(STATUS "-- PCL related stuff will be disabled." )
    endif(PCL_FOUND)
endif(LVR2_WITH_PCL)

#------------------------------------------------------------------------------
# Searching for OpenMP
#------------------------------------------------------------------------------
lvr2_find_package(OpenMP)
if(OPENMP_FOUND)
  message(STATUS "Found OpenMP")
  link_directories(${OpenMP_LIBRARY_DIRS})
  include_directories(${OpenMP_INCLUDE_DIRS})
  list(APPEND LVR2_DEFINITIONS -DLVR2_USE_OPEN_MP)

  # tasks only in OpenMP 3.0 and above thus we need llvm openmp in experimental mode
  # See here: https://devblogs.microsoft.com/cppblog/improved-openmp-support-for-cpp-in-visual-studio/
  if(MSVC)
    set(CMAKE_CXX_FLAGS " -openmp:llvm -openmp:experimental ${CMAKE_CXX_FLAGS}")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif(OPENMP_FOUND)

#------------------------------------------------------------------------------
## Searching for libfreenect
#------------------------------------------------------------------------------
if(LVR2_WITH_FREENECT)
  lvr2_find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBFREENECT libfreenect)
  if(LIBFREENECT_FOUND)
    message(STATUS "Found Freenect")
    include_directories(${LIBFREENECT_INCLUDE_DIRS})
  endif()
endif(LVR2_WITH_FREENECT)

###############################################################################
# USED THIRD PARTY STUFF
###############################################################################

# fmt is part of the public logging facade for the C++17 baseline.
lvr2_find_package(fmt CONFIG REQUIRED)
if(TARGET fmt::fmt)
  set(LVR2_FMT_TARGET fmt::fmt)
elseif(TARGET fmt::fmt-header-only)
  set(LVR2_FMT_TARGET fmt::fmt-header-only)
else()
  message(FATAL_ERROR "fmt package did not provide fmt::fmt or fmt::fmt-header-only")
endif()

# spdlog (package-backed; no vendored fallback)
lvr2_find_package(spdlog CONFIG REQUIRED)
if(TARGET spdlog::spdlog_header_only)
  set(LVR2_SPDLOG_TARGET spdlog::spdlog_header_only)
elseif(TARGET spdlog::spdlog)
  set(LVR2_SPDLOG_TARGET spdlog::spdlog)
else()
  message(FATAL_ERROR "spdlog package did not provide spdlog::spdlog_header_only or spdlog::spdlog")
endif()

# HighFive is used by public headers, so the package target is part of the
# installed lvr2/lvr3 dependency surface.
lvr2_find_package(HighFive CONFIG REQUIRED)
if(TARGET HighFive)
  set(LVR2_HIGHFIVE_TARGET HighFive)
elseif(TARGET HighFive::HighFive)
  set(LVR2_HIGHFIVE_TARGET HighFive::HighFive)
else()
  message(FATAL_ERROR "HighFive package did not provide HighFive or HighFive::HighFive")
endif()

# RPLY and LASlib replace the old vendored mesh/point-cloud I/O helper builds.
lvr2_find_package(rply CONFIG REQUIRED)
if(TARGET rply::rply)
  set(LVR2_RPLY_TARGET rply::rply)
elseif(TARGET rply)
  set(LVR2_RPLY_TARGET rply)
else()
  message(FATAL_ERROR "rply package did not provide rply::rply or rply")
endif()

lvr2_find_package(laslib CONFIG QUIET)
if(NOT TARGET LASlib AND NOT TARGET LASlib::LASlib)
  lvr2_find_package(LASlib CONFIG QUIET)
endif()
if(TARGET LASlib)
  set(LVR2_LASLIB_TARGET LASlib)
elseif(TARGET LASlib::LASlib)
  set(LVR2_LASLIB_TARGET LASlib::LASlib)
else()
  message(FATAL_ERROR "LASlib/lastools package did not provide LASlib or LASlib::LASlib")
endif()

# Compatibility aliases for in-tree tools that still link the historical helper
# target names. They resolve to package targets and are not installed/exported.
if(NOT TARGET lvr2rply_static)
  add_library(lvr2rply_static ALIAS ${LVR2_RPLY_TARGET})
endif()
if(NOT TARGET lvr2rply)
  add_library(lvr2rply ALIAS ${LVR2_RPLY_TARGET})
endif()
if(NOT TARGET lvr2las_static)
  add_library(lvr2las_static ALIAS ${LVR2_LASLIB_TARGET})
endif()
if(NOT TARGET lvr2las)
  add_library(lvr2las ALIAS ${LVR2_LASLIB_TARGET})
endif()

# yaml-cpp
lvr2_find_package(yaml-cpp REQUIRED)
# no found variable
include_directories(${YAML_CPP_INCLUDE_DIR})

# RiVLib remains optional but must come from a package path, not ext/.
set(RiVLib_DIR "" CACHE PATH "Path to a package-provided RiVLib config directory")
if(RiVLib_DIR OR LVR2_USE_SYSTEM_RIVLIB)
    set(RiVLib_USE_STATIC_RUNTIME ON)
    lvr2_find_package(RiVLib COMPONENTS scanifc QUIET)
    if(RiVLib_FOUND)
        include_directories(${RiVLib_INCLUDE_DIRS})
        list(APPEND LVR2_DEFINITIONS -DLVR2_USE_RIVLIB)
        message(STATUS "Found RiVLib")
    elseif(RiVLib_DIR)
        message(FATAL_ERROR "RiVLib_DIR was set but RiVLib was not found")
    endif(RiVLib_FOUND)
endif()

# 3D Tiles support needs a package-backed Cesium Native integration. The old
# ExternalProject download fallback was removed with the vendored dependencies.
if(LVR2_WITH_3DTILES)
  message(FATAL_ERROR
    "LVR2_WITH_3DTILES requires package-backed Cesium Native/Draco support. "
    "The old ExternalProject download fallback was removed in vcpkg-first dependency policy; keep this "
    "option OFF until a non-vendored package path is added.")
else(LVR2_WITH_3DTILES)
  #------------------------------------------------------------------------------
  # Searching for Draco
  #------------------------------------------------------------------------------
  lvr2_find_package(Draco)
  if(draco_FOUND)
    message(STATUS "Found Draco")
    include_directories(${draco_INCLUDE_DIRS})
    list(APPEND LVR2_DEFINITIONS -DLVR2_USE_DRACO)
  endif(draco_FOUND)
endif(LVR2_WITH_3DTILES)

###############################################################################
# ADD LVR DEFINITIONS
###############################################################################

add_definitions(${LVR2_DEFINITIONS})

###############################################################################
# LVR-Kinfu Checks
###############################################################################


# ###############################################################################
# # Check and set CUDA host compiler flags.
# ###############################################################################
if(CUDA_FOUND)
  include("${PROJECT_SOURCE_DIR}/cmake/Lvr3CudaGccVersion.cmake")
  max_cuda_gcc_version(CUDA_VERSION MAX_CUDA_GCC_VERSION)
  message(STATUS "Highest supported GCC version for CUDA: ${MAX_CUDA_GCC_VERSION}")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER ${MAX_CUDA_GCC_VERSION})
      message(STATUS "******************************************************************")
      message(STATUS "* Your gcc version needs to be lower than or equal ${MAX_CUDA_GCC_VERSION} to compile  *")
      message(STATUS "* the CUDA library and apps. Setting CUDA_HOST_COMPILER to       *")
      message(STATUS "* g++-${MAX_CUDA_GCC_VERSION}. Please ensure that g++-${MAX_CUDA_GCC_VERSION} is installed on your system.   *")
      message(STATUS "******************************************************************")
      set(CUDA_HOST_COMPILER "g++-${MAX_CUDA_GCC_VERSION}" CACHE STRING "" FORCE)
    endif()
endif(CUDA_FOUND)


###############################################################################
# APPLE OMP OPTION FOR CLANG-OMP
###############################################################################
if(APPLE AND (NOT OPENMP_FOUND))
        message(STATUS "******************************************************************")
        message(STATUS "* It seems you are trying to comile on OSX with an compiler that *")
        message(STATUS "* does not support OpenMP. To get maximum performance consider   *")
        message(STATUS "* installing clang-omp from homebrew (brew install clang-omp) and*")
        message(STATUS "* configure with 'cmake -DWITH_CLANG_OMP ..'                     *")
        message(STATUS "******************************************************************")

  option(LVR2_WITH_CLANG_OMP "Compile with clang-omp")

  if(LVR2_WITH_CLANG_OMP)
    set(CMAKE_C_COMPILER /usr/local/bin/clang-omp CACHE STRING "C compiler" FORCE)
      set(CMAKE_CXX_COMPILER /usr/local/bin/clang-omp++ CACHE STRING "C++ compiler" FORCE)
      list(APPEND LVR2_DEFINITIONS -DLVR2_USE_OPEN_MP)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
  endif()

endif()


###############################################################################
# SET DEPENDENCIES
###############################################################################

if(MSVC)
set(LVR2_LIB_DEPENDENCIES
    ${Boost_LIBARIES}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${GDAL_LIBRARY}
    ${OpenCV_LIBS}
    ${GSL_LIBRARIES}
    ${LZ4_LIBRARY}
    ${TIFF_LIBRARY}
    ${YAML_CPP_LIBRARIES}
    ${OpenMP_CXX_LIBRARIES}
    ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES}
    Eigen3::Eigen
    )
else()
set(LVR2_LIB_DEPENDENCIES
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${GDAL_LIBRARY}
    ${OpenCV_LIBS}
    ${GSL_LIBRARIES}
    ${LZ4_LIBRARY}
    ${TIFF_LIBRARY}
    ${YAML_CPP_LIBRARIES}
    ${OpenMP_CXX_LIBRARIES}
    ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES}
    Eigen3::Eigen
    m
    ${RDB_LIBRARIES}
    )
endif()

#####################################################################################
# ADD ALL EXTERNAL DEPENDENCIES
#####################################################################################

if(UNIX)
  list(APPEND LVR2_LIB_DEPENDENCIES ${LVR2_LIB_DEPENDENCIES} pthread)
endif(UNIX)

if(PCL_FOUND)
  list(APPEND LVR2_LIB_DEPENDENCIES ${PCL_LIBRARIES} ${MPI_CXX_LIBRARIES})
endif(PCL_FOUND)

if(LIBFREENECT_FOUND)
  list(APPEND LVR2_LIB_DEPENDENCIES ${LIBFREENECT_LIBRARIES})
endif(LIBFREENECT_FOUND)

if(draco_FOUND)
  list(APPEND LVR2_LIB_DEPENDENCIES ${draco_LIBRARIES})
endif(draco_FOUND)

if(RiVLib_FOUND)
  list(APPEND LVR2_LIB_DEPENDENCIES ${RiVLib_SCANIFC_LIBRARY})
endif(RiVLib_FOUND)

if(OPENCL_FOUND)
  list(APPEND LVR2_LIB_DEPENDENCIES ${OpenCL_LIBRARIES})
endif(OPENCL_FOUND)

if(MPI_FOUND)
  set(LVR2_LIB_DEPENDENCIES ${LVR2_LIB_DEPENDENCIES} ${MPI_LIBRARIES})
endif(MPI_FOUND)

if(MSVC)
  list(APPEND LVR2_LIB_DEPENDENCIES yaml-cpp)
endif(MSVC)

if(YAML_CPP_LIBRARIES)
  list(APPEND LVR2_LIB_DEPENDENCIES ${YAML_CPP_LIBRARIES})
endif(YAML_CPP_LIBRARIES)

if(embree_FOUND)
  list(APPEND LVR2_LIB_DEPENDENCIES ${EMBREE_LIBRARY})
endif(embree_FOUND)

if(3DTILES_FOUND)
  list(APPEND LVR2_LIB_DEPENDENCIES ${3DTILES_LIBRARIES})
endif(3DTILES_FOUND)
