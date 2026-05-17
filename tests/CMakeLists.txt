# TESTS
option(LVR2_FETCH_GTEST "Download GoogleTest when a system package is unavailable" OFF)
option(LVR2_ENABLE_PYTEST "Register pytest helper tests when Python and pytest are available" ON)

if(NOT COMMAND lvr2_find_package)
  macro(lvr2_find_package)
    find_package(${ARGV})
  endmacro()
endif()

lvr2_find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
  lvr2_find_package(GTest QUIET)
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

add_executable(lvr2_mesh_io_facade_header_compile
  test_mesh_io_facade_header_compile.cpp
)
target_include_directories(lvr2_mesh_io_facade_header_compile PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(lvr2_mesh_io_facade_header_compile PRIVATE cxx_std_20)
target_link_libraries(lvr2_mesh_io_facade_header_compile PRIVATE tl::expected)
add_test(NAME lvr2_mesh_io_facade_header_compile COMMAND lvr2_mesh_io_facade_header_compile)
set_tests_properties(lvr2_mesh_io_facade_header_compile
  PROPERTIES
    LABELS "cpp;io;mesh;header"
)

add_executable(lvr2_static_assert_contracts_compile
  test_static_assert_contracts.cpp
)
target_include_directories(lvr2_static_assert_contracts_compile PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(lvr2_static_assert_contracts_compile PRIVATE cxx_std_20)
target_link_libraries(lvr2_static_assert_contracts_compile PRIVATE Eigen3::Eigen tl::expected)
add_test(NAME lvr2_static_assert_contracts_compile COMMAND lvr2_static_assert_contracts_compile)
set_tests_properties(lvr2_static_assert_contracts_compile
  PROPERTIES
    LABELS "cpp;contracts;header"
)

add_executable(lvr2_storage_io_header_compile
  test_storage_io_header_compile.cpp
)
target_include_directories(lvr2_storage_io_header_compile PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(lvr2_storage_io_header_compile PRIVATE cxx_std_20)
target_link_libraries(lvr2_storage_io_header_compile PRIVATE tl::expected)
add_test(NAME lvr2_storage_io_header_compile COMMAND lvr2_storage_io_header_compile)
set_tests_properties(lvr2_storage_io_header_compile
  PROPERTIES
    LABELS "cpp;io;scan;storage;header"
)

add_executable(lvr2_mapped_file_contracts
  test_mapped_file.cpp
  ${PROJECT_SOURCE_DIR}/src/liblvr2/util/MappedFile.cpp
)
target_include_directories(lvr2_mapped_file_contracts PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(lvr2_mapped_file_contracts PRIVATE cxx_std_20)
add_test(NAME lvr2_mapped_file_contracts COMMAND lvr2_mapped_file_contracts)
set_tests_properties(lvr2_mapped_file_contracts
  PROPERTIES
    LABELS "cpp;util;boost;cxx20"
)

add_executable(lvr2_cxx20_view_span_contracts
  test_cxx20_view_span_contracts.cpp
)
target_include_directories(lvr2_cxx20_view_span_contracts PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(lvr2_cxx20_view_span_contracts PRIVATE cxx_std_20)
target_link_libraries(lvr2_cxx20_view_span_contracts PRIVATE Eigen3::Eigen ${LVR2_HIGHFIVE_TARGET})
add_test(NAME lvr2_cxx20_view_span_contracts COMMAND lvr2_cxx20_view_span_contracts)
set_tests_properties(lvr2_cxx20_view_span_contracts
  PROPERTIES
    LABELS "cpp;cxx20;views;spans;types"
)

add_executable(lvr2_cli_parser_contracts
  test_cli_parser.cpp
)
target_include_directories(lvr2_cli_parser_contracts PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(lvr2_cli_parser_contracts PRIVATE cxx_std_20)
add_test(NAME lvr2_cli_parser_contracts COMMAND lvr2_cli_parser_contracts)
set_tests_properties(lvr2_cli_parser_contracts
  PROPERTIES
    LABELS "cpp;cxx20;cli;dependencies"
)

if(NOT LVR2_SPDLOG_TARGET)
  find_package(spdlog CONFIG QUIET)
  if(TARGET spdlog::spdlog_header_only)
    set(LVR2_SPDLOG_TARGET spdlog::spdlog_header_only)
  endif()
endif()

if(LVR2_SPDLOG_TARGET)
  add_executable(lvr2_logging_facade_header_compile
    test_logging_facade_header_compile.cpp
  )
  target_include_directories(lvr2_logging_facade_header_compile PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_features(lvr2_logging_facade_header_compile PRIVATE cxx_std_20)
  target_compile_definitions(lvr2_logging_facade_header_compile PRIVATE SPDLOG_USE_STD_FORMAT)
  target_link_libraries(lvr2_logging_facade_header_compile PRIVATE ${LVR2_SPDLOG_TARGET})
  add_test(NAME lvr2_logging_facade_header_compile COMMAND lvr2_logging_facade_header_compile)
  set_tests_properties(lvr2_logging_facade_header_compile
    PROPERTIES
      LABELS "cpp;logging;header"
  )
else()
  message(STATUS "spdlog not found; skipping logging facade header compile test.")
endif()

add_test(
  NAME lvr2_removed_public_mesh_io_headers
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/removed_public_mesh_io_headers.cmake
)
set_tests_properties(lvr2_removed_public_mesh_io_headers
  PROPERTIES
    LABELS "cpp;io;mesh;header"
)

add_test(
  NAME lvr2_no_public_assimp_leakage
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/no_public_assimp_leakage.cmake
)
set_tests_properties(lvr2_no_public_assimp_leakage
  PROPERTIES
    LABELS "cpp;io;mesh;header;assimp"
)

add_test(
  NAME lvr2_no_legacy_mesh_facade_usage
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/no_legacy_mesh_facade_usage.cmake
)
set_tests_properties(lvr2_no_legacy_mesh_facade_usage
  PROPERTIES
    LABELS "cpp;io;mesh;replacement"
)

add_test(
  NAME lvr2_unified_io_namespace_guard
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/unified_io_namespace_guard.cmake
)
set_tests_properties(lvr2_unified_io_namespace_guard
  PROPERTIES
    LABELS "cpp;io;namespace;storage;contracts"
)

add_test(
  NAME lvr2_required_private_assimp_policy
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/required_private_assimp_policy.cmake
)
set_tests_properties(lvr2_required_private_assimp_policy
  PROPERTIES
    LABELS "cmake;io;mesh;assimp"
)

add_test(
  NAME lvr2_no_vendored_dependencies
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/no_vendored_dependencies.cmake
)
set_tests_properties(lvr2_no_vendored_dependencies
  PROPERTIES
    LABELS "cmake;dependencies"
)

add_test(
  NAME lvr2_boost_retirement_inventory
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/boost_retirement_inventory.cmake
)
set_tests_properties(lvr2_boost_retirement_inventory
  PROPERTIES
    LABELS "cmake;dependencies"
)

add_test(
  NAME lvr2_no_boost_cli_parser_dependency
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/no_boost_cli_parser_dependency.cmake
)
set_tests_properties(lvr2_no_boost_cli_parser_dependency
  PROPERTIES
    LABELS "cmake;dependencies;cli"
)

add_test(
  NAME lvr2_boost_stdlib_replacements
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/boost_stdlib_replacements.cmake
)
set_tests_properties(lvr2_boost_stdlib_replacements
  PROPERTIES
    LABELS "cmake;dependencies;boost;cxx20"
)

add_test(
  NAME lvr2_no_boost_dependency
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/no_boost_dependency.cmake
)
set_tests_properties(lvr2_no_boost_dependency
  PROPERTIES
    LABELS "cmake;dependencies;boost;cxx20"
)

add_test(
  NAME lvr2_system_optout_semantics
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/system_optout_semantics.cmake
)
set_tests_properties(lvr2_system_optout_semantics
  PROPERTIES
    LABELS "cmake;dependencies"
)

add_test(
  NAME lvr2_diagnostic_hooks_policy
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/diagnostic_hooks_policy.cmake
)
set_tests_properties(lvr2_diagnostic_hooks_policy
  PROPERTIES
    LABELS "cmake;diagnostics;sanitizer;fuzz;performance"
)

add_test(
  NAME lvr2_format_logging_policy
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/format_logging_policy.cmake
)
set_tests_properties(lvr2_format_logging_policy
  PROPERTIES
    LABELS "cmake;logging;policy"
)

add_test(
  NAME lvr2_packaging_policy
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/packaging_policy.cmake
)
set_tests_properties(lvr2_packaging_policy
  PROPERTIES
    LABELS "cmake;packaging;ros;debian"
)

add_test(
  NAME lvr2_no_bundled_viewer
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/no_bundled_viewer.cmake
)
set_tests_properties(lvr2_no_bundled_viewer
  PROPERTIES
    LABELS "cmake;viewer;dependencies"
)

add_test(
  NAME lvr2_cmake_module_audit
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/cmake_module_audit.cmake
)
set_tests_properties(lvr2_cmake_module_audit
  PROPERTIES
    LABELS "cmake;dependencies;modules"
)

add_test(
  NAME lvr2_source_include_root_guard
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/source_include_root_guard.cmake
)
set_tests_properties(lvr2_source_include_root_guard
  PROPERTIES
    LABELS "cmake;source-layout;includes"
)

add_test(
  NAME lvr2_lyrical_cxx20_baseline
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/lyrical_cxx20_baseline.cmake
)
set_tests_properties(lvr2_lyrical_cxx20_baseline
  PROPERTIES
    LABELS "cmake;ros;lyrical;cxx20"
)

add_test(
  NAME lvr2_cxx20_view_span_policy
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/cxx20_view_span_policy.cmake
)
set_tests_properties(lvr2_cxx20_view_span_policy
  PROPERTIES
    LABELS "cmake;cxx20;views;spans"
)

add_test(
  NAME lvr2_storage_io_contract_guard
  COMMAND ${CMAKE_COMMAND}
    -DLVR2_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_LIST_DIR}/storage_io_contract_guard.cmake
)
set_tests_properties(lvr2_storage_io_contract_guard
  PROPERTIES
    LABELS "cmake;io;scan;storage;contracts"
)

if(LVR2_MESH_IO_ASSIMP_AVAILABLE)
  add_executable(lvr2_assimp_adapter_smoke
    test_assimp_adapter_smoke.cpp
    ${PROJECT_SOURCE_DIR}/src/liblvr2/io/AssimpMeshAdapter.cpp
    ${PROJECT_SOURCE_DIR}/src/liblvr2/types/MeshBuffer.cpp
    ${PROJECT_SOURCE_DIR}/src/liblvr2/types/CustomChannelTypes.cpp
    ${PROJECT_SOURCE_DIR}/src/liblvr2/util/Timestamp.cpp
  )
  target_include_directories(lvr2_assimp_adapter_smoke PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/liblvr2/private
    ${HDF5_INCLUDE_DIRS}
  )
  target_compile_features(lvr2_assimp_adapter_smoke PRIVATE cxx_std_20)
  target_link_libraries(lvr2_assimp_adapter_smoke PRIVATE Eigen3::Eigen ${LVR2_HIGHFIVE_TARGET})
  if(LVR2_ASSIMP_TARGET)
    target_link_libraries(lvr2_assimp_adapter_smoke PRIVATE ${LVR2_ASSIMP_TARGET})
  else()
    target_include_directories(lvr2_assimp_adapter_smoke PRIVATE ${LVR2_ASSIMP_INCLUDE_DIRS})
    target_link_libraries(lvr2_assimp_adapter_smoke PRIVATE ${LVR2_ASSIMP_LIBRARIES})
  endif()
  add_test(NAME lvr2_assimp_adapter_smoke COMMAND lvr2_assimp_adapter_smoke)
  set_tests_properties(lvr2_assimp_adapter_smoke
    PROPERTIES
      LABELS "cpp;io;mesh;assimp"
  )
endif()

if(LVR2_ENABLE_FUZZING)
  add_executable(lvr2_mesh_io_fuzz
    fuzz/mesh_io_fuzz.cpp
  )
  target_include_directories(lvr2_mesh_io_fuzz PRIVATE
    ${PROJECT_SOURCE_DIR}/include
  )
  target_compile_features(lvr2_mesh_io_fuzz PRIVATE cxx_std_20)
  target_link_libraries(lvr2_mesh_io_fuzz PRIVATE lvr2)
  lvr2_configure_fuzz_target(lvr2_mesh_io_fuzz)

  if(LVR2_FUZZ_WITH_LIBFUZZER)
    add_test(
      NAME lvr2_mesh_io_fuzz_seed
      COMMAND lvr2_mesh_io_fuzz -runs=1 ${CMAKE_CURRENT_LIST_DIR}/fuzz/seeds/minimal.obj.seed
    )
  else()
    add_test(
      NAME lvr2_mesh_io_fuzz_seed
      COMMAND lvr2_mesh_io_fuzz ${CMAKE_CURRENT_LIST_DIR}/fuzz/seeds/minimal.obj.seed
    )
  endif()
  set_tests_properties(lvr2_mesh_io_fuzz_seed
    PROPERTIES
      LABELS "fuzz;io;mesh"
      RUN_SERIAL TRUE
      TIMEOUT 30
  )
endif()

if(_LVR2_GTEST_MAIN_TARGET)
  add_executable(lvr2_smoke_gtest
    test_smoke_gtest.cpp
    test_fixtures_seeds_metrics.cpp
  )
  target_include_directories(lvr2_smoke_gtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_features(lvr2_smoke_gtest PRIVATE cxx_std_20)
  target_link_libraries(lvr2_smoke_gtest PRIVATE ${_LVR2_GTEST_MAIN_TARGET})

  add_executable(lvr2_mesh_io_facade_gtest
    test_mesh_io_facade.cpp
    test_mesh_io_replacement.cpp
  )
  target_include_directories(lvr2_mesh_io_facade_gtest PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_features(lvr2_mesh_io_facade_gtest PRIVATE cxx_std_20)
  target_link_libraries(lvr2_mesh_io_facade_gtest PRIVATE
    ${_LVR2_GTEST_MAIN_TARGET}
    lvr2
  )

  add_executable(lvr2_pca_normals_gtest
    test_pca_normals.cpp
  )
  target_include_directories(lvr2_pca_normals_gtest PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_features(lvr2_pca_normals_gtest PRIVATE cxx_std_20)
  target_link_libraries(lvr2_pca_normals_gtest PRIVATE
    ${_LVR2_GTEST_MAIN_TARGET}
    lvr2
  )

  add_executable(lvr2_search_grid_contracts_gtest
    test_search_grid_contracts.cpp
  )
  target_include_directories(lvr2_search_grid_contracts_gtest PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_features(lvr2_search_grid_contracts_gtest PRIVATE cxx_std_20)
  target_link_libraries(lvr2_search_grid_contracts_gtest PRIVATE
    ${_LVR2_GTEST_MAIN_TARGET}
    lvr2
  )

  add_executable(lvr2_storage_project_store_gtest
    test_storage_project_store.cpp
  )
  target_include_directories(lvr2_storage_project_store_gtest PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_features(lvr2_storage_project_store_gtest PRIVATE cxx_std_20)
  target_link_libraries(lvr2_storage_project_store_gtest PRIVATE
    ${_LVR2_GTEST_MAIN_TARGET}
    lvr2
  )

  include(GoogleTest)
  gtest_discover_tests(lvr2_smoke_gtest
    PROPERTIES
      LABELS "cpp;smoke"
  )
  gtest_discover_tests(lvr2_mesh_io_facade_gtest
    PROPERTIES
      LABELS "cpp;io;mesh"
  )
  gtest_discover_tests(lvr2_pca_normals_gtest
    PROPERTIES
      LABELS "cpp;geometry;normals"
  )
  gtest_discover_tests(lvr2_search_grid_contracts_gtest
    PROPERTIES
      LABELS "cpp;geometry;search;grid"
  )
  gtest_discover_tests(lvr2_storage_project_store_gtest
    PROPERTIES
      LABELS "cpp;io;scan;storage"
  )

  add_executable(lvr2_mesh_io_assimp_gtest
    test_mesh_io_assimp.cpp
  )
  target_include_directories(lvr2_mesh_io_assimp_gtest PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/liblvr2/private
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_features(lvr2_mesh_io_assimp_gtest PRIVATE cxx_std_20)
  target_link_libraries(lvr2_mesh_io_assimp_gtest PRIVATE
    ${_LVR2_GTEST_MAIN_TARGET}
    lvr2
  )
  gtest_discover_tests(lvr2_mesh_io_assimp_gtest
    PROPERTIES
      LABELS "cpp;io;mesh;assimp"
  )
else()
  message(STATUS "GoogleTest targets not available; skipping C++ smoke test target.")
endif()

if(LVR2_ENABLE_PERFORMANCE_BASELINES)
  lvr2_find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(
      NAME lvr2_performance_baseline_smoke
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_LIST_DIR}/performance/record_baseline.py
        --output ${CMAKE_BINARY_DIR}/performance-baselines/smoke-baseline.json
        --label ctest-generated-fixture-smoke
    )
    set_tests_properties(lvr2_performance_baseline_smoke
      PROPERTIES
        LABELS "performance;baseline"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
  else()
    message(FATAL_ERROR "LVR2_ENABLE_PERFORMANCE_BASELINES=ON requires a Python3 interpreter.")
  endif()
endif()

if(LVR2_ENABLE_PYTEST)
  lvr2_find_package(Python3 COMPONENTS Interpreter QUIET)
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
