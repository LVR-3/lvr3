# OPTIONS
option(LVR2_BUILD_EXAMPLES "Build the examples" OFF)
option(LVR2_BUILD_TOOLS "Build tools including lvr2_reconstruct" ON)
option(LVR2_BUILD_TOOLS_EXPERIMENTAL "Build experimental tools" OFF)
option(LVR2_WITH_KINFU "Compile LVR Kinfu" OFF)
option(LVR2_WITH_3DTILES "Compile with 3DTiles support" OFF)
option(LVR2_WITH_CUDA "Compile with CUDA support, if available" ON)
option(LVR2_WITH_OPENCL "Compile with OpenCL support, if available" ON)
option(LVR2_WITH_PCL "Compile with PCL support" OFF)
option(LVR2_WITH_FREENECT "Compile with libfreenect grabber" OFF)
option(LVR2_WITH_CV_NONFREE "Use OpenCV non-free descriptors" OFF)

# Static-first output policy:
#  - build static libraries by default
#  - build shared libraries only when explicitly enabled
option(LVR2_BUILD_STATIC_LIBS "Build static lvr2 libraries" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(_LVR2_DEFAULT_MESH_ASSET_IO OFF)
if(BUILD_SHARED_LIBS AND NOT LVR2_BUILD_STATIC_LIBS)
  set(_LVR2_DEFAULT_MESH_ASSET_IO ON)
endif()
option(LVR2_ENABLE_MESH_ASSET_IO "Enable required private mesh asset I/O; shared-only to avoid exporting the private backend through static targets" ${_LVR2_DEFAULT_MESH_ASSET_IO})
unset(_LVR2_DEFAULT_MESH_ASSET_IO)

# Test and diagnostics options
option(LVR2_BUILD_TESTS "Build lvr2 tests" OFF)
option(LVR2_ENABLE_SANITIZERS "Opt in to compiler sanitizer instrumentation for developer/CI diagnostics" OFF)
set(LVR2_SANITIZERS "address;undefined" CACHE STRING "Semicolon-separated sanitizer list: address;undefined;leak;thread;memory")
option(LVR2_ENABLE_FUZZING "Build opt-in fuzz targets and seed-corpus CTest hooks" OFF)
option(LVR2_FUZZ_WITH_LIBFUZZER "Link fuzz targets with Clang libFuzzer instead of the standalone seed runner" OFF)
option(LVR2_ENABLE_PERFORMANCE_BASELINES "Register non-gating performance baseline recording tests" OFF)
if(NOT DEFINED BUILD_TESTING)
  set(BUILD_TESTING ${LVR2_BUILD_TESTS} CACHE BOOL "Enable testing")
endif()
if((LVR2_BUILD_TESTS OR LVR2_ENABLE_FUZZING OR LVR2_ENABLE_PERFORMANCE_BASELINES) AND NOT BUILD_TESTING)
  set(BUILD_TESTING ON CACHE BOOL "Enable testing" FORCE)
endif()

if(NOT LVR2_BUILD_STATIC_LIBS AND NOT BUILD_SHARED_LIBS)
  message(FATAL_ERROR "Both static/shared output are disabled. Set either LVR2_BUILD_STATIC_LIBS=ON or BUILD_SHARED_LIBS=ON.")
endif()

if(LVR2_ENABLE_MESH_ASSET_IO AND (NOT BUILD_SHARED_LIBS OR LVR2_BUILD_STATIC_LIBS))
  message(FATAL_ERROR
    "Required private mesh asset I/O is shared-only. Configure with "
    "-DBUILD_SHARED_LIBS=ON -DLVR2_BUILD_STATIC_LIBS=OFF, or set "
    "-DLVR2_ENABLE_MESH_ASSET_IO=OFF for non-mesh static/policy builds.")
endif()
