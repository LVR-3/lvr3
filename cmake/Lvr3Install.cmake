###############################################################################
# HEADERS
###############################################################################

install(DIRECTORY include/lvr2 DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
list(APPEND LVR2_INSTALL_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})

###############################################################################
# CMAKE FILES
###############################################################################
include(CMakePackageConfigHelpers)
set(SYSCONFIG_INSTALL_DIR etc/lvr2/)

set(_LVR_CMAKE_MODULES
    # Bounded compatibility exceptions documented in docs/cmake/module-audit.md.
    cmake/modules/FindFLANN.cmake
    cmake/modules/FindLZ4.cmake
)

install(EXPORT lvr2-targets
    FILE lvr2-targets.cmake
    NAMESPACE lvr2::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lvr2)

# The lvr3 package is canonical but intentionally imports the legacy lvr2 target
# export names as compatibility targets before adding lvr3 aliases.
install(EXPORT lvr2-targets
    FILE lvr2-targets.cmake
    NAMESPACE lvr2::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lvr3)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/lvr2-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/lvr3-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_file(cmake/lvr-dependencies.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/lvr-dependencies.cmake
    @ONLY
)

configure_package_config_file(cmake/lvr2-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/lvr2-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lvr2
)

configure_package_config_file(cmake/lvr3-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/lvr3-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lvr3
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lvr2-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/lvr2-config-version.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/lvr-dependencies.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lvr2
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lvr3-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/lvr3-config-version.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/lvr-dependencies.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lvr3
)

# install package.xml for ROS under the canonical package identity.
install(FILES package.xml DESTINATION share/lvr3)

install(FILES
    ${_LVR_CMAKE_MODULES}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lvr2/Modules)

install(FILES
    ${_LVR_CMAKE_MODULES}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lvr3/Modules)

###############################################################################
# Uninstall
###############################################################################
# https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#can-i-do-make-uninstall-with-cmake
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lvr2-uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/lvr2-uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/lvr2-uninstall.cmake
    COMMENT "Uninstall lvr2 libraries and all header files")
endif()
