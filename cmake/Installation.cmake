# Installation.cmake - Configure installation rules

# Include files
install(DIRECTORY include/ DESTINATION include)

# Library files
install(TARGETS formula
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# CMake config for external projects (optional, for future use)
# install(EXPORT formulaTargets DESTINATION lib/cmake/formula)

message(STATUS "Installation configured: include/ -> include/, libformula.a -> lib/")
