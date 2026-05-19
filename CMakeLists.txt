cmake_minimum_required(VERSION 4.2)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Lvr3DependencyProvider.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Lvr3DependencyPolicy.cmake")

project(lvr3 VERSION 25.2.3)

# Compatibility variables for legacy target/output names that intentionally
# remain lvr2 while the public package identity moves to lvr3.
set(lvr2_VERSION ${PROJECT_VERSION})
set(lvr2_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(lvr2_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(lvr2_VERSION_PATCH ${PROJECT_VERSION_PATCH})

include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Options.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3ProjectSettings.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Testing.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Dependencies.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Targets.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Install.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Documentation.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Packaging.cmake")
