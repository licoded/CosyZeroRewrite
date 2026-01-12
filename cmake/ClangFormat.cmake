# ============================================================
# ClangFormat - Code formatting
# ============================================================

find_program(CLANG_FORMAT_EXE clang-format)

if(CLANG_FORMAT_EXE)
    message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")

    # 收集所有需要格式化的源文件
    file(GLOB_RECURSE
        ALL_CXX_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c
    )

    # 排除 deps 和 build 目录
    list(FILTER ALL_CXX_SOURCE_FILES EXCLUDE REGEX ".*deps/.*")
    list(FILTER ALL_CXX_SOURCE_FILES EXCLUDE REGEX ".*build/.*")

    # 添加格式化目标（直接修改文件）
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_CXX_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting code with clang-format..."
        VERBATIM
    )

    # 添加检查目标（不修改文件，只检查）
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ALL_CXX_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking code format with clang-format..."
        VERBATIM
    )

    # 为单个文件添加格式化目标
    foreach(SOURCE_FILE ${ALL_CXX_SOURCE_FILES})
        file(RELATIVE_PATH REL_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR} ${SOURCE_FILE})
        string(REPLACE "/" "-" TARGET_NAME ${REL_SOURCE_FILE})
        string(REPLACE "." "-" TARGET_NAME ${TARGET_NAME})
        add_custom_target(format-${TARGET_NAME}
            COMMAND ${CLANG_FORMAT_EXE} -i ${SOURCE_FILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Formatting ${REL_SOURCE_FILE}..."
            VERBATIM
        )
    endforeach()

    # 添加帮助信息
    set(FORMAT_HELP
        "Code formatting targets:"
        "  make format          - Format all source files"
        "  make format-check    - Check if code is properly formatted"
        "  make format-<file>   - Format a specific file (e.g., format-include-formula-formula-hpp)"
    )

    message(STATUS "${FORMAT_HELP}")
else()
    message(WARNING "clang-format not found. Code formatting targets will not be available.")
    message(STATUS "  Install with: brew install clang-format (macOS)")
    message(STATUS "               or: apt install clang-format (Ubuntu)")

    # 创建空目标（防止 make format 报错）
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found. Please install clang-format."
    )
    add_custom_target(format-check
        COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found. Please install clang-format."
    )
endif()
