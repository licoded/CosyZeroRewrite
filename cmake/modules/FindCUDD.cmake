# FindCUDD.cmake - Find CUDD BDD library
#
# This module defines:
#  CUDD_FOUND - True if CUDD was found
#  CUDD_INCLUDE_DIRS - Include directories for CUDD
#  CUDD_LIBRARIES - Libraries to link against
#  CUDD_VERSION - Version of CUDD (if available)

find_path(CUDD_INCLUDE_DIRS
    NAMES cudd.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/cudd/include
        $ENV{CUDD_DIR}/include
        ${CMAKE_SOURCE_DIR}/deps/cudd
    DOC "CUDD include directory"
)

find_library(CUDD_LIBRARIES
    NAMES cudd cudd_obj
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/cudd/lib
        $ENV{CUDD_DIR}/lib
        ${CMAKE_SOURCE_DIR}/deps/cudd
    DOC "CUDD library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CUDD
    REQUIRED_VARS CUDD_INCLUDE_DIRS CUDD_LIBRARIES
)

if(CUDD_FOUND)
    message(STATUS "CUDD found: ${CUDD_INCLUDE_DIRS}")
    message(STATUS "CUDD library: ${CUDD_LIBRARIES}")
else()
    message(WARNING "CUDD not found. BDD optimizations will be disabled.")
    message(STATUS "  To install: sudo apt install libcudd-dev (Ubuntu/Debian)")
    message(STATUS "  Or build from source: https://github.com/ivmai/cudd")
endif()

mark_as_advanced(CUDD_INCLUDE_DIRS CUDD_LIBRARIES)
