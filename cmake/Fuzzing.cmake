# Fuzzing.cmake - Configure libFuzzer targets

option(BUILD_FUZZER "Build libFuzzer targets" OFF)

if(BUILD_FUZZER)
    # Check if compiler supports -fsanitize=fuzzer (requires clang)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-fsanitize=fuzzer HAS_FUZZER_FLAG)

    if(HAS_FUZZER_FLAG)
        message(STATUS "LibFuzzer enabled")

        # ============================================================
        # Parser fuzzer - tests formula string parsing
        # ============================================================
        add_executable(parser_fuzzer tests/fuzz/fuzz_parser.cpp)
        target_compile_options(parser_fuzzer PRIVATE -fsanitize=fuzzer -g -O1)
        target_link_libraries(parser_fuzzer PRIVATE formula)

        # ============================================================
        # Transformation property fuzzer - tests semantic preservation
        # ============================================================
        add_executable(transformation_fuzzer tests/fuzz/fuzz_transformations.cpp)
        target_compile_options(transformation_fuzzer PRIVATE -fsanitize=fuzzer -g -O1)
        target_link_libraries(transformation_fuzzer PRIVATE formula)

        # ============================================================
        # Equivalence fuzzer - tests equivalence checker correctness
        # ============================================================
        add_executable(equivalence_fuzzer tests/fuzz/fuzz_equivalence.cpp)
        target_compile_options(equivalence_fuzzer PRIVATE -fsanitize=fuzzer -g -O1)
        target_link_libraries(equivalence_fuzzer PRIVATE formula)

        message(STATUS "Fuzzer targets: parser_fuzzer, transformation_fuzzer, equivalence_fuzzer")

    else()
        message(WARNING "Compiler does not support -fsanitize=fuzzer. Building fallback targets.")

        # ============================================================
        # Standalone test harness for external fuzzing tools
        # ============================================================
        add_executable(fuzz_harness tests/fuzz/fuzz_harness.cpp)
        target_link_libraries(fuzz_harness PRIVATE formula)

        # Test driver that simulates fuzzer behavior
        add_executable(fuzz_driver tests/fuzz/fuzz_driver.cpp)
        target_link_libraries(fuzz_driver PRIVATE formula)

        message(STATUS "Fallback targets: fuzz_harness, fuzz_driver")
        message(STATUS "To use libFuzzer with clang:")
        message(STATUS "  mkdir -p build_fuzz && cd build_fuzz")
        message(STATUS "  CC=clang CXX=clang++ cmake .. -DBUILD_FUZZER=ON")
    endif()
endif()
