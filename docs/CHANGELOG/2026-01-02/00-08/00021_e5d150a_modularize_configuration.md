# [21] Refactor CMake: modularize configuration

**Commit**: `e5d150a` ([`e5d150a28ea34e74b11b556e9354c51fab945e95`](https://github.com/anthropics/cosy-zero/commit/e5d150a28ea34e74b11b556e9354c51fab945e95))
**Date**: 2026-01-02 02:14:03 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Split CMakeLists.txt (176 lines -> 66 lines)
- Move dependency detection to cmake/Dependencies.cmake
- Create cmake/modules/ for FindXXX.cmake files
  - FindSpdlog.cmake: spdlog detection
  - FindZ3.cmake: Z3 SMT solver detection
- Separate concerns into dedicated modules:
  - CompilerOptions.cmake: compiler flags
  - LibraryTargets.cmake: library build
  - Tests.cmake: unit test configuration
  - Fuzzing.cmake: libFuzzer targets
  - Installation.cmake: install rules
- Add claude.md with CMake architecture documentation
- Update .gitignore to exclude generated .cmake but keep source ones

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `claude.md`
- `cmake/CompilerOptions.cmake`
- `cmake/Dependencies.cmake`
- `cmake/Fuzzing.cmake`
- `cmake/Installation.cmake`
- `cmake/LibraryTargets.cmake`
- `cmake/modules/FindSpdlog.cmake`
- `cmake/modules/FindZ3.cmake`
- `cmake/Tests.cmake`


### Modified
- `CMakeLists.txt`
- `.gitignore`


## Stats

- **11** files changed
- **496** insertions(+)
- **169** deletions(-)
