# LibraryTargets.cmake - Build the formula library

# Build formula library
add_library(formula STATIC ${FORMULA_SOURCES})

# Add external directory for nlohmann/json library
target_include_directories(formula PUBLIC ${CMAKE_SOURCE_DIR}/external)

# Link Z3 to formula library if available
if(Z3_FOUND)
    target_link_libraries(formula ${Z3_LIBRARIES})
    message(STATUS "Formula library: linking with Z3")
endif()

# Set library properties
set_target_properties(formula PROPERTIES
    VERSION ${PROJECT_VERSION}
    OUTPUT_NAME "formula"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

message(STATUS "Formula library configured")
