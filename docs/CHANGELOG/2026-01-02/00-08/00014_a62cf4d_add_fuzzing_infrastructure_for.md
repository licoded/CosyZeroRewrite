# [14] Add fuzzing infrastructure for formula library

**Commit**: `a62cf4d` ([`a62cf4d3abc57c75562ca6f598f197c810687143`](https://github.com/anthropics/cosy-zero/commit/a62cf4d3abc57c75562ca6f598f197c810687143))
**Date**: 2026-01-02 01:31:49 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Fuzzer targets (require clang with libFuzzer):
- parser_fuzzer: Tests formula string parsing
- transformation_fuzzer: Tests transformation invariants
- equivalence_fuzzer: Tests equivalence checker properties

Fallback for GCC (no libFuzzer):
- fuzz_harness: Standalone harness for AFL++ or other fuzzers
- fuzz_driver: Test driver that simulates fuzzer behavior

Organized fuzz code in tests/fuzz/ subdirectory

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `tests/fuzz/fuzz_driver.cpp`
- `tests/fuzz/fuzz_equivalence.cpp`
- `tests/fuzz/fuzz_harness.cpp`
- `tests/fuzz/fuzz_parser.cpp`
- `tests/fuzz/fuzz_transformations.cpp`


### Modified
- `CMakeLists.txt`


## Stats

- **6** files changed
- **888** insertions(+)
- **0** deletions(-)
