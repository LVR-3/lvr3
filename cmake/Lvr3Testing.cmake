include(CTest)

include("${PROJECT_SOURCE_DIR}/cmake/Lvr3SanitizerFuzz.cmake")
lvr2_configure_sanitizer_hooks()
