# Dependencies.cmake - Find and configure all project dependencies

# ============================================================
# Base formula library sources
# ============================================================
set(FORMULA_SOURCES
    src/formula/formula.cpp
    src/formula/formula_pool.cpp
    src/formula/formula_parser.cpp
    src/formula/formula_checker.cpp
    src/formula/nnf.cpp
    src/formula/xnf.cpp
    src/formula/simplify.cpp
    src/formula/rmnext.cpp
    src/automata/dfa.cpp
    src/automata/tableau.cpp
    src/synthesis/synthesis.cpp
    src/synthesis/game_solver.cpp
    src/synthesis/on_the_fly_solver.cpp
    src/synthesis/game_graph_export.cpp
    src/synthesis/strategy.cpp
    src/synthesis/trace_exporter.cpp
)

# ============================================================
# spdlog (logging library)
# ============================================================
option(USE_SPDLOG "Enable spdlog logging" ON)

if(USE_SPDLOG)
    # First check for local copy in project
    if(EXISTS "${CMAKE_SOURCE_DIR}/deps/spdlog/spdlog.h")
        message(STATUS "spdlog found: local copy in deps/")
        include_directories("${CMAKE_SOURCE_DIR}/deps")
        add_compile_definitions(FORMULA_USE_LOGGER)
        set(SPDLOG_FOUND TRUE)
    else()
        # Use custom Find module
        find_package(spdlog QUIET)

        if(SPDLOG_FOUND)
            message(STATUS "spdlog found: ${SPDLOG_INCLUDE_DIRS}")
            include_directories(${SPDLOG_INCLUDE_DIRS})
            add_compile_definitions(FORMULA_USE_LOGGER)
        else()
            message(WARNING "spdlog not found. Logging will be disabled.")
            message(STATUS "  To install: sudo apt install libspdlog-dev (Ubuntu/Debian)")
            message(STATUS "             brew install spdlog (macOS)")
        endif()
    endif()
endif()

# ============================================================
# Z3 (SMT solver)
# ============================================================
option(USE_Z3 "Enable Z3 SMT solver for equivalence checking" ON)

if(USE_Z3)
    # Use custom Find module
    find_package(Z3 QUIET)

    if(Z3_FOUND)
        message(STATUS "Z3 found: ${Z3_LIBRARIES}")
        include_directories(${Z3_INCLUDE_DIRS})
        add_compile_definitions(FORMULA_USE_Z3)

        # Add Z3 source file to FORMULA_SOURCES
        list(APPEND FORMULA_SOURCES src/formula/formula_z3.cpp)
    else()
        message(WARNING "Z3 requested but not found. Z3 support will be disabled.")
        message(STATUS "  To install: sudo apt install libz3-dev (Ubuntu/Debian)")
        message(STATUS "             brew install z3 (macOS)")
    endif()
endif()
