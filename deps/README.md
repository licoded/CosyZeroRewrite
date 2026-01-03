# External Dependencies

This directory contains third-party libraries used by CosyZeroRewrite.

## catch2/

**Purpose**: Header-only test framework for C++
**Version**: v2.13.9
**License**: BSL-2.0 (Boost Software License)
**Source**: https://github.com/catchorg/Catch2
**Usage**: Unit testing (see `tests/` directory)

## spdlog/

**Purpose**: Fast C++ logging library
**Version**: v1.12.0 (header-only)
**License**: MIT
**Source**: https://github.com/gabime/spdlog
**Usage**: Logging throughout the codebase

### Adding New Dependencies

1. Create a subdirectory here
2. Copy/update this README with:
   - Purpose
   - Version
   - License
   - Source URL
   - Usage notes
3. Update CMake to reference `deps/<name>/`
