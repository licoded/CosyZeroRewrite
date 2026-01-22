# [169] refactor: reorganize tests directory and standardize CMake output

**Commit**: `d617261` ([`d617261a146dd647bac3ddccb88fb6d55527c9f4`](https://github.com/licoded/CosyZeroRewrite/commit/d617261a146dd647bac3ddccb88fb6d55527c9f4))
**Date**: 2026-01-04 01:20:38 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Major restructuring:

Tests directory:
- Grouped by category: formula/, parser/, transformation/, automata/, synthesis/
  integration/, fuzz/, debug/, bench/
- Removed _test suffix from file names
- Removed fuzz_ prefix from fuzz files

Dependencies:
- Moved tests/catch2/ to deps/catch2/
- Added deps/README.md for dependency documentation

Output structure (build/):
- output/          - Tools (Cosy2, benchmark_runner)
- tests/           - Test executables mirroring tests/ structure
- libformula.a     - Static library

CMake improvements:
- Added helper functions add_cosy_test() and add_cosy_test_standalone()
- Configured output directories in CMakeLists.txt
- Test targets renamed to avoid conflicts (e.g., formula_test)
- Added options: BUILD_DEBUG_TESTS, BUILD_BENCH_TESTS

Documentation:
- Added tests/README.md explaining test structure
- Added docs/ARCHITECTURE/cmake_usage.md for CMake usage

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
<!-- TODO: Add a brief summary of the change in Chinese or English -->

### 🔍 Technical Details
<!-- Optional: Add technical details, root cause, or implementation notes -->

### 📊 Impact Analysis
<!-- Optional: Add impact scope, affected components, or performance notes -->

## Changes

### Added
- `deps/catch2/catch.hpp`
- `deps/README.md`
- `docs/ARCHITECTURE/cmake_usage.md`
- `tests/automata/dfa.cpp`
- `tests/automata/tableau.cpp.disabled`
- `tests/automata/tarjan.cpp`
- `tests/bench/benchmark.cpp`
- `tests/bench/stress.cpp`
- `tests/debug/eventually_contradiction.cpp`
- `tests/debug/failing.cpp`
- `tests/formula/formula.cpp`
- `tests/formula/transformation.cpp`
- `tests/fuzz/driver.cpp`
- `tests/fuzz/equivalence.cpp`
- `tests/fuzz/harness.cpp`
- `tests/fuzz/nnf.cpp`
- `tests/fuzz/parser.cpp`
- `tests/fuzz/random.cpp`
- `tests/fuzz/transformations.cpp`
- `tests/fuzz/xnf.cpp`
- `tests/integration/io_separation.cpp`
- `tests/integration/prop_atoms.cpp`
- `tests/integration/strategy.cpp`
- `tests/parser/parser.cpp`
- `tests/README.md`
- `tests/synthesis/on_the_fly.cpp`
- `tests/synthesis/synthesis.cpp`
- `tests/transformation/next.cpp`
- `tests/transformation/nnf.cpp`
- `tests/transformation/xnf.cpp`


### Modified
- `CMakeLists.txt`
- `cmake/Tests.cmake`


### Deleted
- `tests/benchmark_runner.cpp`
- `tests/catch2/catch.hpp`
- `tests/debug_eventually_contradiction.cpp`
- `tests/debug_failing_tests.cpp`
- `tests/dfa_test.cpp`
- `tests/formula_tests.cpp`
- `tests/fuzz/fuzz_driver.cpp`
- `tests/fuzz/fuzz_equivalence.cpp`
- `tests/fuzz/fuzz_harness.cpp`
- `tests/fuzz/fuzz_parser.cpp`
- `tests/fuzz/fuzz_transformations.cpp`
- `tests/io_separation_test.cpp`
- `tests/next_tests.cpp`
- `tests/nnf_fuzz_test.cpp`
- `tests/nnf_tests.cpp`
- `tests/on_the_fly_synthesis_tests.cpp`
- `tests/parser_checker_tests.cpp`
- `tests/prop_atoms_test.cpp`
- `tests/random_formula_test.cpp`
- `tests/strategy_extraction_test.cpp`
- `tests/stress_test.cpp`
- `tests/synthesis_test.cpp`
- `tests/tableau_state_test.cpp.disabled`
- `tests/tarjan_scc_tests.cpp`
- `tests/transformation_tests.cpp`
- `tests/xnf_fuzz_test.cpp`
- `tests/xnf_tests.cpp`


## Stats

- **59** files changed
- **26303** insertions(+)
- **26025** deletions(-)
