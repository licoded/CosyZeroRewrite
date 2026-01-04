# Tests.cmake - Configure testing with Catch2
#
# Standardized CMake usage:
# - Tests output to: build/tests/<category>/
# - Tools output to: build/output/
# - Dependencies in: deps/

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_STRESS_TEST "Build stress test (long-running)" OFF)
option(BUILD_DEBUG_TESTS "Build debug tests (for development)" ON)
option(BUILD_BENCH_TESTS "Build benchmark tests" ON)

# ============================================================================
# Dependencies
# ============================================================================
if(BUILD_TESTS)
    # Catch2 header-only library
    add_library(catch2 INTERFACE)
    target_include_directories(catch2 INTERFACE ${PROJECT_SOURCE_DIR}/deps/testing/catch2)

    enable_testing()
    message(STATUS "Testing enabled - deps/catch2")
endif()

# ============================================================================
# Helper: Define test with output directory and compile options
# ============================================================================
function(add_cosy_test test_name category source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE formula catch2)

    # Output to build/tests/<category>/
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/${category}
    )

    # Compile options
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4)
    else()
        target_compile_options(${test_name} PRIVATE -Wno-sign-compare)
    endif()
endfunction()

# ============================================================================
# Helper: Define standalone test (no Catch2)
# ============================================================================
function(add_cosy_test_standalone test_name category source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE formula)
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/deps/runtime)

    # Output to build/tests/<category>/
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/${category}
    )

    # Compile options
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4)
    else()
        target_compile_options(${test_name} PRIVATE -Wno-sign-compare)
    endif()
endfunction()

# ============================================================================
# Formula tests
# ============================================================================
if(BUILD_TESTS)
    add_cosy_test(formula_test formula tests/formula/formula.cpp)
    add_cosy_test(transformation_test formula tests/formula/transformation.cpp)
endif()

# ============================================================================
# Parser tests
# ============================================================================
if(BUILD_TESTS)
    add_cosy_test(parser_test parser tests/parser/parser.cpp)
endif()

# ============================================================================
# Transformation tests
# ============================================================================
if(BUILD_TESTS)
    add_cosy_test(nnf_test transformation tests/transformation/nnf.cpp)
    add_cosy_test(xnf_test transformation tests/transformation/xnf.cpp)
endif()

# ============================================================================
# Automata tests
# ============================================================================
if(BUILD_TESTS)
    add_cosy_test(dfa_test automata tests/automata/dfa.cpp)
    add_cosy_test(tarjan_test automata tests/automata/tarjan.cpp)
    add_cosy_test(progression_test automata tests/automata/progression.cpp)
endif()

# ============================================================================
# Synthesis tests
# ============================================================================
if(BUILD_TESTS)
    add_cosy_test(synthesis_test synthesis tests/synthesis/synthesis.cpp)
    add_cosy_test(on_the_fly_test synthesis tests/synthesis/on_the_fly.cpp)
endif()

# ============================================================================
# Integration tests
# ============================================================================
if(BUILD_TESTS)
    add_cosy_test(prop_atoms_test integration tests/integration/prop_atoms.cpp)
endif()

# ============================================================================
# Fuzz tests (standalone, no Catch2)
# ============================================================================
if(BUILD_TESTS)
    add_cosy_test_standalone(nnf_fuzz fuzz tests/fuzz/nnf.cpp)
    add_cosy_test_standalone(xnf_fuzz fuzz tests/fuzz/xnf.cpp)
    add_cosy_test_standalone(random_fuzz fuzz tests/fuzz/random.cpp)
endif()

# ============================================================================
# Debug tests (standalone, for development)
# ============================================================================
if(BUILD_DEBUG_TESTS)
    add_cosy_test_standalone(eventually_contradiction debug tests/debug/eventually_contradiction.cpp)
    add_cosy_test_standalone(failing_tests debug tests/debug/failing.cpp)
    message(STATUS "Debug tests enabled")
endif()

# ============================================================================
# Bench tests
# ============================================================================
if(BUILD_BENCH_TESTS)
    add_cosy_test_standalone(benchmark_test bench tests/bench/benchmark.cpp)
    add_cosy_test_standalone(io_separation_test integration tests/integration/io_separation.cpp)
    add_cosy_test_standalone(strategy_test integration tests/integration/strategy.cpp)
    message(STATUS "Bench tests enabled")
endif()

# ============================================================================
# Stress test (separate option, runs for hours)
# ============================================================================
if(BUILD_STRESS_TEST)
    add_cosy_test_standalone(stress_test bench tests/bench/stress.cpp)
    message(STATUS "Stress test enabled")
endif()

# ============================================================================
# Register tests with CTest
# ============================================================================
if(BUILD_TESTS)
    # Core formula tests
    add_test(NAME formula_test COMMAND ${CMAKE_BINARY_DIR}/tests/formula/formula_test)
    add_test(NAME transformation_test COMMAND ${CMAKE_BINARY_DIR}/tests/formula/transformation_test)
    add_test(NAME parser_test COMMAND ${CMAKE_BINARY_DIR}/tests/parser/parser_test)

    # Transformation tests
    add_test(NAME nnf_test COMMAND ${CMAKE_BINARY_DIR}/tests/transformation/nnf_test)
    add_test(NAME xnf_test COMMAND ${CMAKE_BINARY_DIR}/tests/transformation/xnf_test)

    # Automata tests
    add_test(NAME dfa_test COMMAND ${CMAKE_BINARY_DIR}/tests/automata/dfa_test)
    add_test(NAME tarjan_test COMMAND ${CMAKE_BINARY_DIR}/tests/automata/tarjan_test)
    add_test(NAME progression_test COMMAND ${CMAKE_BINARY_DIR}/tests/automata/progression_test)

    # Synthesis tests
    add_test(NAME synthesis_test COMMAND ${CMAKE_BINARY_DIR}/tests/synthesis/synthesis_test)
    add_test(NAME on_the_fly_test COMMAND ${CMAKE_BINARY_DIR}/tests/synthesis/on_the_fly_test)

    # Integration tests
    add_test(NAME prop_atoms_test COMMAND ${CMAKE_BINARY_DIR}/tests/integration/prop_atoms_test)

    message(STATUS "=== Test Structure ===")
    message(STATUS "  tests/formula/     - formula_test, transformation_test")
    message(STATUS "  tests/parser/      - parser_test")
    message(STATUS "  tests/transformation/ - nnf_test, xnf_test")
    message(STATUS "  tests/automata/    - dfa_test, tarjan_test, progression_test")
    message(STATUS "  tests/synthesis/   - synthesis_test, on_the_fly_test")
    message(STATUS "  tests/integration/ - prop_atoms_test, io_separation_test, strategy_test")
    message(STATUS "  tests/fuzz/        - nnf_fuzz, xnf_fuzz, random_fuzz")
    message(STATUS "  tests/debug/       - eventually_contradiction, failing_tests")
    message(STATUS "  tests/bench/       - benchmark_test, stress_test")
    message(STATUS "====================")
endif()
