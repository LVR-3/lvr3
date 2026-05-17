cmake_minimum_required(VERSION 4.2)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Lvr3DependencyProvider.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Lvr3DependencyPolicy.cmake")

project(lvr2 VERSION 25.2.3)

include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Options.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3ProjectSettings.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Testing.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Dependencies.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Targets.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Install.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Documentation.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Lvr3Packaging.cmake")
