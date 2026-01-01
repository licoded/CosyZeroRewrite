# CompilerOptions.cmake - Set compiler options for the project

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)

    # Debug-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    endif()

    # Release-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    endif()

    message(STATUS "Compiler warnings enabled: Wall, Wextra, Wpedantic")
elseif(MSVC)
    add_compile_options(/W4)

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2)
    endif()

    message(STATUS "Compiler warnings enabled: /W4")
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
message(STATUS "Include directories configured")
