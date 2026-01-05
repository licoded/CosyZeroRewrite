# Dependencies

This directory contains libraries used by CosyZeroRewrite.

## Structure

```
deps/
├── external/    # Third-party libraries
│   ├── catch2/      # Testing framework
│   ├── CLI/         # Command-line parsing
│   ├── indicators/  # Progress bars
│   ├── nlohmann/    # JSON library
│   └── spdlog/      # Logging
└── internal/    # Internal utilities (reserved for future use)
```

## External Libraries

### catch2/
- **Purpose**: Header-only test framework
- **Version**: v2.13.9
- **License**: BSL-2.0
- **Source**: https://github.com/catchorg/Catch2

### CLI/
- **Purpose**: Command-line argument parsing
- **Version**: v2.3.2
- **License**: BSL-2.0
- **Source**: https://github.com/CLIUtils/CLI11

### indicators/
- **Purpose**: Progress bars and spinners
- **Version**: v2.3
- **License**: MIT
- **Source**: https://github.com/p-ranav/indicators

### nlohmann/
- **Purpose**: JSON parsing and generation
- **Version**: v3.11.3
- **License**: MIT
- **Source**: https://github.com/nlohmann/json

### spdlog/
- **Purpose**: Fast C++ logging library
- **Version**: v1.12.0 (header-only)
- **License**: MIT
- **Source**: https://github.com/gabime/spdlog

## Adding New Dependencies

1. Create subdirectory under `deps/external/`
2. Update this README with:
   - Purpose
   - Version
   - License
   - Source URL
3. Update CMakeLists.txt if needed
