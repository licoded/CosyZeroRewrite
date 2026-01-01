cmake_minimum_required(VERSION 3.10)
project(CosyZeroFormula VERSION 1.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files for formula library
set(FORMULA_SOURCES
    src/formula/formula.cpp
    src/formula/formula_pool.cpp
    src/formula/formula_parser.cpp
    src/formula/formula_checker.cpp
    src/formula/nnf.cpp
    src/formula/xnf.cpp
    src/formula/simplify.cpp
    src/formula/rmnext.cpp
)

# Build formula library
add_library(formula STATIC ${FORMULA_SOURCES})

# Test executable
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()

    # Catch2 header-only library
    add_library(catch2 INTERFACE)
    target_include_directories(catch2 INTERFACE ${PROJECT_SOURCE_DIR}/tests/catch2)

    # Original tests (with CATCH_CONFIG_MAIN)
    add_executable(formula_tests tests/formula_tests.cpp)
    target_link_libraries(formula_tests PRIVATE formula catch2)

    # Parser and checker tests (combined with formula_tests for main)
    add_executable(parser_checker_tests tests/formula_tests.cpp tests/parser_checker_tests.cpp)
    target_link_libraries(parser_checker_tests PRIVATE formula catch2)

    # Disable warnings for Catch2
    if(MSVC)
        target_compile_options(formula_tests PRIVATE /W4)
        target_compile_options(parser_checker_tests PRIVATE /W4)
    else()
        target_compile_options(formula_tests PRIVATE -Wno-sign-compare)
        target_compile_options(parser_checker_tests PRIVATE -Wno-sign-compare)
    endif()

    add_test(NAME formula_tests COMMAND formula_tests)
    add_test(NAME parser_checker_tests COMMAND parser_checker_tests)
endif()

# Fuzzing with libFuzzer
option(BUILD_FUZZER "Build libFuzzer targets" OFF)
if(BUILD_FUZZER)
    # Check if compiler supports -fsanitize=fuzzer (requires clang)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-fsanitize=fuzzer HAS_FUZZER_FLAG)

    if(HAS_FUZZER_FLAG)
        # Parser fuzzer - tests formula string parsing
        add_executable(parser_fuzzer tests/fuzz/fuzz_parser.cpp)
        target_compile_options(parser_fuzzer PRIVATE -fsanitize=fuzzer -g -O1)
        target_link_libraries(parser_fuzzer PRIVATE formula)

        # Transformation property fuzzer - tests semantic preservation
        add_executable(transformation_fuzzer tests/fuzz/fuzz_transformations.cpp)
        target_compile_options(transformation_fuzzer PRIVATE -fsanitize=fuzzer -g -O1)
        target_link_libraries(transformation_fuzzer PRIVATE formula)

        # Equivalence fuzzer - tests equivalence checker correctness
        add_executable(equivalence_fuzzer tests/fuzz/fuzz_equivalence.cpp)
        target_compile_options(equivalence_fuzzer PRIVATE -fsanitize=fuzzer -g -O1)
        target_link_libraries(equivalence_fuzzer PRIVATE formula)

        message(STATUS "LibFuzzer targets enabled: parser_fuzzer, transformation_fuzzer, equivalence_fuzzer")
    else()
        # Build standalone test harness for fuzzing with external tools
        add_executable(fuzz_harness tests/fuzz/fuzz_harness.cpp)
        target_link_libraries(fuzz_harness PRIVATE formula)

        # Build test driver that simulates fuzzer behavior
        add_executable(fuzz_driver tests/fuzz/fuzz_driver.cpp)
        target_link_libraries(fuzz_driver PRIVATE formula)

        message(WARNING "Compiler does not support -fsanitize=fuzzer. Built fuzz_harness and fuzz_driver instead.")
        message(STATUS "To run fuzzers with clang+libFuzzer, install clang with libFuzzer support:")
        message(STATUS "  Ubuntu: sudo apt install clang libfuzzer-dev")
        message(STATUS "Then:")
        message(STATUS "  mkdir -p build_fuzz && cd build_fuzz")
        message(STATUS "  CC=clang CXX=clang++ cmake .. -DBUILD_FUZZER=ON")
    endif()
endif()

# Installation
install(TARGETS formula
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)
