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
option(USE_Z3 "Enable Z3 SMT solver" ON)
option(USE_SPDLOG "Enable spdlog logging" ON)

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
# Build library
# ============================================================
include(cmake/LibraryTargets.cmake)

# ============================================================
# Testing
# ============================================================
include(cmake/Tests.cmake)

# ============================================================
# Fuzzing
# ============================================================
include(cmake/Fuzzing.cmake)

# ============================================================
# Installation
# ============================================================
include(cmake/Installation.cmake)

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
message(STATUS "========================================")
