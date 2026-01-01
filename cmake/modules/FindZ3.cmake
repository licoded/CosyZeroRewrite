# FindZ3.cmake - Find Z3 SMT solver
#
# This module defines:
#   Z3_FOUND - True if Z3 is found
#   Z3_INCLUDE_DIRS - Include directories for Z3
#   Z3_LIBRARIES - Libraries to link
#   Z3_VERSION - Version of Z3 (if available)

include(FindPackageHandleStandardArgs)

# Try pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(Z3_PKG libz3 QUIET)
    if(Z3_PKG_FOUND)
        set(Z3_INCLUDE_DIRS ${Z3_PKG_INCLUDE_DIRS})
        set(Z3_LIBRARIES ${Z3_PKG_LIBRARIES})
        set(Z3_VERSION ${Z3_PKG_VERSION})
    endif()
endif()

# Fallback: manual search
if(NOT Z3_FOUND)
    # Find header
    find_path(Z3_INCLUDE_DIR
        NAMES z3++.h z3.h
        PATHS
            /usr/include
            /usr/local/include
            /opt/homebrew/include
            C:/libs/z3/include
        DOC "Z3 include directory"
    )

    # Find library
    find_library(Z3_LIBRARY
        NAMES z3 libz3
        PATHS
            /usr/lib
            /usr/local/lib
            /usr/lib/x86_64-linux-gnu
            /opt/homebrew/lib
            C:/libs/z3/lib
        DOC "Z3 library"
    )

    if(Z3_INCLUDE_DIR AND Z3_LIBRARY)
        set(Z3_INCLUDE_DIRS ${Z3_INCLUDE_DIR})
        set(Z3_LIBRARIES ${Z3_LIBRARY})
    endif()
endif()

# Handle find_package arguments
find_package_handle_standard_args(Z3
    REQUIRED_VARS Z3_INCLUDE_DIRS Z3_LIBRARIES
    VERSION_VAR Z3_VERSION
)

# Cache results
if(Z3_FOUND)
    set(Z3_FOUND TRUE CACHE INTERNAL "Z3 found")
    set(Z3_INCLUDE_DIRS ${Z3_INCLUDE_DIRS} CACHE STRING "Z3 include directories")
    set(Z3_LIBRARIES ${Z3_LIBRARIES} CACHE STRING "Z3 libraries")
endif()

# Provide imported target
if(Z3_FOUND AND NOT TARGET Z3::libz3)
    add_library(Z3::libz3 UNKNOWN IMPORTED)
    set_target_properties(Z3::libz3 PROPERTIES
        IMPORTED_LOCATION "${Z3_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${Z3_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(Z3_INCLUDE_DIR Z3_LIBRARY)
