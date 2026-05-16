###############################################################################
# DOCUMENTATION
###############################################################################
lvr2_find_package(Doxygen)
if( NOT DOXYGEN_FOUND )
  message( WARNING "Could not find Doxygen. "
      "You will not be able to build the documentation." )
endif( NOT DOXYGEN_FOUND )

if( NOT DOXYGEN_DOT_FOUND )
  message( WARNING "Could not find GraphViz. "
      "You will not be able to build the documentation." )
endif( NOT DOXYGEN_DOT_FOUND )

if( DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND )
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
      ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY )
  add_custom_target( doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM )
endif( DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND )
