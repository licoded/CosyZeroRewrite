# Tests.cmake - Configure testing with Catch2

option(BUILD_TESTS "Build unit tests" ON)

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
    add_executable(parser_checker_tests
        tests/formula_tests.cpp
        tests/parser_checker_tests.cpp
    )
    target_link_libraries(parser_checker_tests PRIVATE formula catch2)

    # ============================================================
    # Compiler-specific options for tests
    # ============================================================
    if(MSVC)
        target_compile_options(formula_tests PRIVATE /W4)
        target_compile_options(parser_checker_tests PRIVATE /W4)
    else()
        target_compile_options(formula_tests PRIVATE -Wno-sign-compare)
        target_compile_options(parser_checker_tests PRIVATE -Wno-sign-compare)
    endif()

    # ============================================================
    # Register tests with CTest
    # ============================================================
    add_test(NAME formula_tests COMMAND formula_tests)
    add_test(NAME parser_checker_tests COMMAND parser_checker_tests)

    message(STATUS "Test executables: formula_tests, parser_checker_tests")
endif()
