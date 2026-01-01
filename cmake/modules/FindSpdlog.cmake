# FindSpdlog.cmake - Find spdlog logging library
#
# This module defines:
#   SPDLOG_FOUND - True if spdlog is found
#   SPDLOG_INCLUDE_DIRS - Include directories for spdlog
#   SPDLOG_LIBRARIES - Libraries to link (empty for header-only)
#   SPDLOG_VERSION - Version of spdlog (if available)

include(FindPackageHandleStandardArgs)

# Try pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SPDLOG_PKG spdlog QUIET)
    if(SPDLOG_PKG_FOUND)
        set(SPDLOG_INCLUDE_DIRS ${SPDLOG_PKG_INCLUDE_DIRS})
        set(SPDLOG_LIBRARIES ${SPDLOG_PKG_LIBRARIES})
        set(SPDLOG_VERSION ${SPDLOG_PKG_VERSION})
    endif()
endif()

# Fallback: manual search
if(NOT SPDLOG_FOUND)
    find_path(SPDLOG_INCLUDE_DIR
        NAMES spdlog/spdlog.h
        PATH_SUFFIXES spdlog
        DOC "spdlog include directory"
    )

    if(SPDLOG_INCLUDE_DIR)
        set(SPDLOG_INCLUDE_DIRS ${SPDLOG_INCLUDE_DIR})
        set(SPDLOG_LIBRARIES "")  # Header-only
        set(SPDLOG_VERSION "header-only")
    endif()
endif()

# Handle find_package arguments
find_package_handle_standard_args(spdlog
    REQUIRED_VARS SPDLOG_INCLUDE_DIRS
    VERSION_VAR SPDLOG_VERSION
)

# Cache results
if(SPDLOG_FOUND)
    # For backward compatibility
    set(SPDLOG_FOUND TRUE CACHE INTERNAL "spdlog found")
    set(SPDLOG_INCLUDE_DIRS ${SPDLOG_INCLUDE_DIRS} CACHE STRING "spdlog include directories")
    set(SPDLOG_LIBRARIES ${SPDLOG_LIBRARIES} CACHE STRING "spdlog libraries")
endif()

# Provide imported target if using modern CMake
if(SPDLOG_FOUND AND NOT TARGET spdlog::spdlog)
    add_library(spdlog::spdlog INTERFACE IMPORTED)
    set_target_properties(spdlog::spdlog PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(SPDLOG_INCLUDE_DIR)
