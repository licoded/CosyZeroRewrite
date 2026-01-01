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

    add_executable(formula_tests tests/formula_tests.cpp)
    target_link_libraries(formula_tests PRIVATE formula catch2)

    # Disable warnings for Catch2
    if(MSVC)
        target_compile_options(formula_tests PRIVATE /W4)
    else()
        target_compile_options(formula_tests PRIVATE -Wno-sign-compare)
    endif()

    add_test(NAME formula_tests COMMAND formula_tests)
endif()

# Installation
install(TARGETS formula
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)
