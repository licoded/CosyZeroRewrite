# Tests.cmake - Configure testing with Catch2

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_STRESS_TEST "Build stress test (long-running)" OFF)

if(BUILD_TESTS)
    enable_testing()

    # Catch2 header-only library
    add_library(catch2 INTERFACE)
    target_include_directories(catch2 INTERFACE ${PROJECT_SOURCE_DIR}/tests/catch2)

    message(STATUS "Testing enabled")

    # ============================================================
    # Original formula tests
    # ============================================================
    add_executable(formula_tests tests/formula_tests.cpp)
    target_link_libraries(formula_tests PRIVATE formula catch2)

    # ============================================================
    # Parser and checker tests
    # ============================================================
    add_executable(parser_checker_tests tests/parser_checker_tests.cpp)
    target_link_libraries(parser_checker_tests PRIVATE formula catch2)

    # ============================================================
    # Transformation equivalence tests
    # ============================================================
    add_executable(transformation_tests tests/transformation_tests.cpp)
    target_link_libraries(transformation_tests PRIVATE formula catch2)

    # ============================================================
    # DFA construction tests
    # ============================================================
    add_executable(dfa_tests tests/dfa_test.cpp)
    target_link_libraries(dfa_tests PRIVATE formula catch2)

    # ============================================================
    # Tableau state tests (unit tests for TableauState)
    # ============================================================
    add_executable(tableau_state_tests tests/tableau_state_test.cpp)
    target_link_libraries(tableau_state_tests PRIVATE formula catch2)

    # ============================================================
    # Synthesis tests
    # ============================================================
    add_executable(synthesis_tests tests/synthesis_test.cpp)
    target_link_libraries(synthesis_tests PRIVATE formula catch2)

    # ============================================================
    # On-the-fly synthesis tests
    # ============================================================
    add_executable(on_the_fly_synthesis_tests tests/on_the_fly_synthesis_tests.cpp)
    target_link_libraries(on_the_fly_synthesis_tests PRIVATE formula)

    # ============================================================
    # Tarjan SCC algorithm tests
    # ============================================================
    add_executable(tarjan_scc_tests tests/tarjan_scc_tests.cpp)
    target_link_libraries(tarjan_scc_tests PRIVATE formula catch2)

    # ============================================================
    # I/O separation tests
    # ============================================================
    add_executable(io_separation_test tests/io_separation_test.cpp)
    target_link_libraries(io_separation_test PRIVATE formula)

    # ============================================================
    # Strategy extraction tests
    # ============================================================
    add_executable(strategy_extraction_test tests/strategy_extraction_test.cpp)
    target_link_libraries(strategy_extraction_test PRIVATE formula)

    # ============================================================
    # Compiler-specific options for tests
    # ============================================================
    if(MSVC)
        target_compile_options(formula_tests PRIVATE /W4)
        target_compile_options(parser_checker_tests PRIVATE /W4)
        target_compile_options(transformation_tests PRIVATE /W4)
        target_compile_options(dfa_tests PRIVATE /W4)
        target_compile_options(tableau_state_tests PRIVATE /W4)
        target_compile_options(synthesis_tests PRIVATE /W4)
        target_compile_options(on_the_fly_synthesis_tests PRIVATE /W4)
        target_compile_options(tarjan_scc_tests PRIVATE /W4)
        target_compile_options(io_separation_test PRIVATE /W4)
        target_compile_options(strategy_extraction_test PRIVATE /W4)
    else()
        target_compile_options(formula_tests PRIVATE -Wno-sign-compare)
        target_compile_options(parser_checker_tests PRIVATE -Wno-sign-compare)
        target_compile_options(transformation_tests PRIVATE -Wno-sign-compare)
        target_compile_options(dfa_tests PRIVATE -Wno-sign-compare)
        target_compile_options(tableau_state_tests PRIVATE -Wno-sign-compare)
        target_compile_options(synthesis_tests PRIVATE -Wno-sign-compare)
        target_compile_options(on_the_fly_synthesis_tests PRIVATE -Wno-sign-compare)
        target_compile_options(tarjan_scc_tests PRIVATE -Wno-sign-compare)
        target_compile_options(io_separation_test PRIVATE -Wno-sign-compare)
        target_compile_options(strategy_extraction_test PRIVATE -Wno-sign-compare)
    endif()

    # ============================================================
    # Register tests with CTest
    # ============================================================
    add_test(NAME formula_tests COMMAND formula_tests)
    add_test(NAME parser_checker_tests COMMAND parser_checker_tests)
    add_test(NAME transformation_tests COMMAND transformation_tests)
    add_test(NAME dfa_tests COMMAND dfa_tests)
    add_test(NAME tableau_state_tests COMMAND tableau_state_tests)
    add_test(NAME synthesis_tests COMMAND synthesis_tests)
    add_test(NAME on_the_fly_synthesis_tests COMMAND on_the_fly_synthesis_tests)
    add_test(NAME tarjan_scc_tests COMMAND tarjan_scc_tests)
    add_test(NAME io_separation_test COMMAND io_separation_test)
    add_test(NAME strategy_extraction_test COMMAND strategy_extraction_test)

    message(STATUS "Test executables: formula_tests, parser_checker_tests, transformation_tests, dfa_tests, tableau_state_tests, synthesis_tests, on_the_fly_synthesis_tests, tarjan_scc_tests, io_separation_test, strategy_extraction_test")
endif()

# ============================================================
# Stress Test (separate option as it runs for hours)
# ============================================================
if(BUILD_STRESS_TEST)
    add_executable(stress_test tests/stress_test.cpp)
    target_link_libraries(stress_test PRIVATE formula)

    if(MSVC)
        target_compile_options(stress_test PRIVATE /W4)
    else()
        target_compile_options(stress_test PRIVATE -Wno-sign-compare)
    endif()

    message(STATUS "Stress test enabled: stress_test")
endif()
