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
# Benchmark runner
# ============================================================
if(BUILD_BENCHMARK)
    add_executable(benchmark_runner tools/benchmark_runner.cpp)
    target_include_directories(benchmark_runner PRIVATE include)
    target_link_libraries(benchmark_runner PRIVATE formula)
    message(STATUS "Benchmark runner enabled")
endif()

# ============================================================
# Cosy2 Synthesis Tool
# ============================================================
add_executable(Cosy2 src/cosy2.cpp)
target_include_directories(Cosy2 PRIVATE include)
target_link_libraries(Cosy2 PRIVATE formula)
# Install Cosy2 to bin directory
install(TARGETS Cosy2 DESTINATION bin)
message(STATUS "Cosy2 synthesis tool enabled")

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
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/changelog.sh ${COMMIT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating CHANGELOG entry for commit ${COMMIT}"
    VERBATIM
)

# Add a target to install the post-commit hook
add_custom_target(install-changelog-hook
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/scripts/post-commit-hook.sh
        ${CMAKE_SOURCE_DIR}/.git/hooks/post-commit
    COMMAND chmod +x ${CMAKE_SOURCE_DIR}/.git/hooks/post-commit
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Installing post-commit hook for CHANGELOG generation"
    VERBATIM
)
