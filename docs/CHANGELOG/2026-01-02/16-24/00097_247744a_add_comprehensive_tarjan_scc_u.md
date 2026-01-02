# [97] test(synthesis): add comprehensive Tarjan SCC unit tests

**Commit**: `247744a` ([`247744ac20e0406c93f9deb7d44694faf5eb23c3`](https://github.com/licoded/CosyZeroRewrite/commit/247744ac20e0406c93f9deb7d44694faf5eb23c3))
**Date**: 2026-01-02 20:05:47 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add tarjan_scc_tests.cpp with 13 test cases covering:
  * Single node, linear chain, simple cycle
  * Two connected cycles, self-loop
  * Complex DAG, cross pattern
  * SCC with incoming/outgoing edges
  * Topological order property

- Add find_sccs_for_testing() as separate implementation
  that doesn't trigger expand_state(), keeping production
  code unchanged (option 2 approach)

- Fix add_test_transition() to append rather than replace
  successor lists

- Add comprehensive algorithm documentation:
  * docs/ARCHITECTURE/on_the_fly_algorithm.md - Complete
    rewrite with correct algorithm understanding
  * docs/ARCHITECTURE/tarjan_scc_algorithm.md - Tarjan
    algorithm research and implementation analysis

- Update docs/BUGS/fixed.md with Bug #003 details

All tests pass: 13/13 test cases, 48 assertions

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
- `docs/ARCHITECTURE/on_the_fly_algorithm.md`
- `docs/ARCHITECTURE/tarjan_scc_algorithm.md`
- `tests/tarjan_scc_tests.cpp`


### Modified
- `cmake/Tests.cmake`
- `docs/BUGS/fixed.md`
- `include/synthesis/on_the_fly_solver.hpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **7** files changed
- **1761** insertions(+)
- **56** deletions(-)
