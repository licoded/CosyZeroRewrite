# [17] Add Z3 SMT solver integration for exact equivalence checking

**Commit**: `06d65ba` ([`06d65baafe811f439d1e5be8953716fa65edfcdc`](https://github.com/anthropics/cosy-zero/commit/06d65baafe811f439d1e5be8953716fa65edfcdc))
**Date**: 2026-01-02 01:46:45 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Features:
- FormulaZ3 class with are_equivalent(), is_valid(), is_satisfiable()
- FormulaChecker::are_equivalent_smart() for automatic method selection
- Supports formulas with any number of variables (beyond 4-variable limit)
- Configurable timeout support (default 5000ms)
- CMake auto-detection for Z3 library with fallback stubs

Implementation details:
- Z3 headers included outside namespace to avoid std:: conflicts
- Uses pointer-based expression storage (z3::expr*) for unordered_map
- Single-point semantics for temporal operators (conservative approximation)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `include/formula/formula_z3.hpp`
- `src/formula/formula_z3.cpp`


### Modified
- `CMakeLists.txt`
- `include/formula/formula_checker.hpp`
- `src/formula/formula_checker.cpp`


## Stats

- **5** files changed
- **466** insertions(+)
- **0** deletions(-)
