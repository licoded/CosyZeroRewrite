cmake_minimum_required(VERSION 3.10)

project(CosyZeroFormula
    VERSION 1.0
    DESCRIPTION "LTLf Formula Library with Z3 Integration"
    LANGUAGES CXX
)

# ============================================================
# Project options
# ============================================================
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_FUZZER "Build libFuzzer targets" OFF)
option(BUILD_BENCHMARK "Build benchmark runner" ON)
option(USE_Z3 "Enable Z3 SMT solver" ON)
option(USE_SPDLOG "Enable spdlog logging" ON)
option(FORCE_STATIC_DEPS "Force static Z3/CUDD for static builds" OFF)

# ============================================================
# Output directories (standardized)
# ============================================================
# - Tools (Cosy2, benchmark_runner) -> build/output/
# - Tests -> build/tests/<category>/
# - Library -> build/libformula.a
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

# ============================================================
# Include subdirectories for CMake modules
# ============================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# ============================================================
# Compiler setup
# ============================================================
include(cmake/CompilerOptions.cmake)

# ============================================================
# Dependencies (Z3, spdlog)
# ============================================================
include(cmake/Dependencies.cmake)

# ============================================================
# Optional: force static Z3/CUDD for fully static builds
# ============================================================
if(FORCE_STATIC_DEPS)
    set(Z3_STATIC_DIR "/home/lic/dev/ltlfsyn-deps/z3-4.8.12-x64-glibc-2.31")
    set(Z3_STATIC_LIB "${Z3_STATIC_DIR}/bin/libz3.a")
    set(Z3_STATIC_INCLUDE "${Z3_STATIC_DIR}/include")

    set(CUDD_STATIC_LIB "/usr/local/lib/libcudd.a")
    set(CUDD_STATIC_INCLUDE "/usr/local/include")

    if(EXISTS "${Z3_STATIC_LIB}" AND EXISTS "${CUDD_STATIC_LIB}")
        set(Z3_FOUND TRUE)
        set(Z3_LIBRARIES "${Z3_STATIC_LIB}")
        set(Z3_INCLUDE_DIRS "${Z3_STATIC_INCLUDE}")
        set(CUDD_FOUND TRUE)
        set(CUDD_LIBRARIES "${CUDD_STATIC_LIB}")
        set(CUDD_INCLUDE_DIRS "${CUDD_STATIC_INCLUDE}")
        message(STATUS "Forcing static Z3/CUDD libraries for formula")
    else()
        message(WARNING "FORCE_STATIC_DEPS enabled, but static Z3/CUDD not found")
    endif()
endif()

# ============================================================
# Build library
# ============================================================
include(cmake/LibraryTargets.cmake)

# ============================================================
# Testing
# ============================================================
include(cmake/Tests.cmake)

# ============================================================
# Code Formatting
# ============================================================
include(cmake/ClangFormat.cmake)

# ============================================================
# Fuzzing
# ============================================================
include(cmake/Fuzzing.cmake)

# ============================================================
# Installation
# ============================================================
include(cmake/Installation.cmake)


# ============================================================
# Cosy2 Synthesis Tool
# ============================================================
add_executable(Cosy2 src/cosy2.cpp)
target_include_directories(Cosy2 PRIVATE include deps/external)
target_link_libraries(Cosy2 PRIVATE formula)
# Install Cosy2 to bin directory
install(TARGETS Cosy2 DESTINATION bin)
message(STATUS "Cosy2 synthesis tool enabled")

# ============================================================
# Cosy2-static - Statically linked version
# ============================================================
# Z3 static library path (for static linking)
set(Z3_STATIC_DIR "/home/lic/dev/ltlfsyn-deps/z3-4.8.12-x64-glibc-2.31")
set(Z3_STATIC_LIB "${Z3_STATIC_DIR}/bin/libz3.a")
set(Z3_STATIC_INCLUDE "${Z3_STATIC_DIR}/include")

# CUDD static library
set(CUDD_STATIC_LIB "/usr/local/lib/libcudd.a")
set(CUDD_STATIC_INCLUDE "/usr/local/include")

if(EXISTS "${Z3_STATIC_LIB}" AND EXISTS "${CUDD_STATIC_LIB}")
    # Create static libraries for linking
    add_library(z3-static STATIC IMPORTED GLOBAL)
    set_target_properties(z3-static PROPERTIES
        IMPORTED_LOCATION "${Z3_STATIC_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${Z3_STATIC_INCLUDE}"
    )

    add_library(cudd-static STATIC IMPORTED GLOBAL)
    set_target_properties(cudd-static PROPERTIES
        IMPORTED_LOCATION "${CUDD_STATIC_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${CUDD_STATIC_INCLUDE}"
    )

    add_executable(Cosy2-static src/cosy2.cpp)
    target_include_directories(Cosy2-static PRIVATE include deps/external)

    # Link with static libraries using -Bstatic/-Bdynamic
    target_link_libraries(Cosy2-static PRIVATE
        formula
        z3-static
        cudd-static
        pthread
        dl
        m
    )
    target_link_options(Cosy2-static PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
        LINKER:--as-needed
    )
    message(STATUS "Cosy2-static (with static Z3, CUDD, stdc++) enabled")
else()
    if(NOT EXISTS "${Z3_STATIC_LIB}")
        message(WARNING "Z3 static library not found at ${Z3_STATIC_LIB}")
    endif()
    if(NOT EXISTS "${CUDD_STATIC_LIB}")
        message(WARNING "CUDD static library not found at ${CUDD_STATIC_LIB}")
    endif()
    message(STATUS "  Cosy2-static will NOT be built")
endif()

# ============================================================
# Summary
# ============================================================
message(STATUS "========================================")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Z3 support: ${Z3_FOUND}")
message(STATUS "spdlog support: ${SPDLOG_FOUND}")
message(STATUS "Tests enabled: ${BUILD_TESTS}")
message(STATUS "Fuzzer enabled: ${BUILD_FUZZER}")
message(STATUS "Benchmark enabled: ${BUILD_BENCHMARK}")
message(STATUS "========================================")

# ============================================================
# Changelog utilities
# ============================================================
# Add a custom target for generating changelog entries
add_custom_target(changelog
    COMMAND ${CMAKE_SOURCE_DIR}/tools/scripts/changelog.sh ${COMMIT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating CHANGELOG entry for commit ${COMMIT}"
    VERBATIM
)

# Add a target to install the post-commit hook
add_custom_target(install-changelog-hook
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/tools/scripts/post-commit-hook.sh
        ${CMAKE_SOURCE_DIR}/.git/hooks/post-commit
    COMMAND chmod +x ${CMAKE_SOURCE_DIR}/.git/hooks/post-commit
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Installing post-commit hook for CHANGELOG generation"
    VERBATIM
)
