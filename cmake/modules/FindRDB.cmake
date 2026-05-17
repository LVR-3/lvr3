unset(RDB_FOUND)
unset(RDB_INCLUDE_DIRS)
unset(RDB_LIBRARY_DIRS)
unset(RDB_LIBRARIES)

mark_as_advanced(RDB_FOUND)
mark_as_advanced(RDB_INCLUDE_DIRS)
mark_as_advanced(RDB_LIBRARY_DIRS)
mark_as_advanced(RDB_LIBRARIES)

get_filename_component(RDB_ROOT_DIR ${_IMPORT_PREFIX}/ DIRECTORY)
get_filename_component(RDB_ROOT_DIR ${RDB_ROOT_DIR}/ DIRECTORY)

# Set rdb_INCLUDE_DIRS
find_path(RDB_INCLUDE_DIRS NAMES "riegl/rdb.hpp" HINTS "/usr/include/ /usr/local/include/")

find_library(RDB_LIBRARY_IMPORT NAMES rdb.dll librdb.dylib librdb.so)
get_filename_component(RDB_LIBRARY_DIRS ${RDB_LIBRARY_IMPORT} DIRECTORY)

if(RDB_INCLUDE_DIRS AND RDB_LIBRARY_IMPORT)
    set(RDB_FOUND YES)
    set(RDB_LIBRARIES rdbc rdbcpp ${RDB_LIBRARY_IMPORT})
endif()

# create imported target rdbc
if(TARGET rdbc)
elseif(RDB_FOUND)
    add_library(rdbc SHARED IMPORTED GLOBAL)

    set_property(TARGET rdbc PROPERTY IMPORTED_LOCATION "${RDB_LIBRARY_IMPORT}")
    set_property(TARGET rdbc PROPERTY IMPORTED_IMPLIB   "${RDB_LIBRARY_IMPORT}")

    # add libraries for rdbc
    set_property(TARGET rdbc PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES
            "${RDB_INCLUDE_DIRS}"
            )
endif()

# create compiled target rdbcpp
if(TARGET rdbcpp)
elseif(RDB_FOUND)
    add_library(rdbcpp STATIC
            "${RDB_INCLUDE_DIRS}/riegl/rdb.cpp"
            )
    # add libraries for rdbcpp
    target_include_directories(rdbcpp SYSTEM PUBLIC
            "${RDB_INCLUDE_DIRS}"
            )
    target_link_libraries(rdbcpp PUBLIC ${RDB_LIBRARY_IMPORT})
    if(NOT MSVC AND NOT MINGW)
        set_target_properties(rdbcpp PROPERTIES COMPILE_FLAGS "-fPIC")
    endif()
endif()



if(TARGET rdbcpp)
    target_compile_features(rdbcpp PUBLIC cxx_std_20)
endif()


