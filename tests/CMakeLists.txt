# TESTS
option(LVR2_FETCH_GTEST "Download GoogleTest when a system package is unavailable" OFF)
option(LVR2_ENABLE_PYTEST "Register pytest helper tests when Python and pytest are available" ON)

find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
  find_package(GTest QUIET)
endif()

if(NOT GTest_FOUND)
  if(LVR2_FETCH_GTEST)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )
    FetchContent_MakeAvailable(googletest)
  else()
    message(STATUS "GoogleTest not found and LVR2_FETCH_GTEST is OFF; skipping C++ smoke test registration.")
  endif()
endif()

set(_LVR2_GTEST_MAIN_TARGET "")
if(TARGET GTest::gtest_main)
  set(_LVR2_GTEST_MAIN_TARGET GTest::gtest_main)
elseif(TARGET GTest::Main)
  set(_LVR2_GTEST_MAIN_TARGET GTest::Main)
endif()

if(_LVR2_GTEST_MAIN_TARGET)
  add_executable(lvr2_smoke_gtest
    test_smoke_gtest.cpp
    test_fixtures_seeds_metrics.cpp
  )
  target_include_directories(lvr2_smoke_gtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_features(lvr2_smoke_gtest PRIVATE cxx_std_17)
  target_link_libraries(lvr2_smoke_gtest PRIVATE ${_LVR2_GTEST_MAIN_TARGET})

  include(GoogleTest)
  gtest_discover_tests(lvr2_smoke_gtest
    PROPERTIES
      LABELS "cpp;smoke"
  )
else()
  message(STATUS "GoogleTest targets not available; skipping C++ smoke test target.")
endif()

if(LVR2_ENABLE_PYTEST)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    execute_process(
      COMMAND ${Python3_EXECUTABLE} -m pytest --version
      RESULT_VARIABLE _LVR2_PYTEST_RESULT
      OUTPUT_VARIABLE _LVR2_PYTEST_OUTPUT
      ERROR_VARIABLE _LVR2_PYTEST_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
    )

    if(_LVR2_PYTEST_RESULT EQUAL 0)
      add_test(
        NAME lvr2_pytest_smoke
        COMMAND ${Python3_EXECUTABLE} -m pytest -q ${CMAKE_CURRENT_SOURCE_DIR}/pytest
      )
      set_tests_properties(
        lvr2_pytest_smoke
        PROPERTIES
          LABELS "python;smoke"
          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      )
    else()
      message(STATUS "pytest was not found in Python interpreter; skipping Python smoke test registration.")
    endif()
  else()
    message(STATUS "Python3 interpreter not found; skipping Python smoke test registration.")
  endif()
endif()

# Package identity helper checks for lvr2/lvr3 compatibility and load order.
add_test(
  NAME lvr2_lvr3_package_identity_smoke
  COMMAND ${CMAKE_COMMAND}
    -P ${CMAKE_CURRENT_LIST_DIR}/package_identity/package_identity_verify.cmake
)
set_tests_properties(
  lvr2_lvr3_package_identity_smoke
  PROPERTIES
    LABELS "cpp;smoke;package-identity"
)
