# [75] feat: add strategy extraction for LTLf synthesis

**Commit**: `776bf70` ([`776bf700bfe94b7714a0de5190b4dc21b4db4541`](https://github.com/licoded/CosyZeroRewrite/commit/776bf700bfe94b7714a0de5190b4dc21b4db4541))
**Date**: 2026-01-02 13:06:23 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implements strategy extraction from classified game graphs, allowing
users to visualize and verify winning strategies for realizable formulas.

Features:
- extract_strategy() method builds strategy graph via BFS from initial state
- find_winning_output() identifies system outputs that lead to winning states
- to_json() exports strategy as JSON for programmatic access
- to_dot() exports strategy as Graphviz DOT for visualization
- StrategyVerifier checks extracted strategy correctness

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `include/synthesis/strategy.hpp`
- `src/synthesis/strategy.cpp`
- `tests/strategy_extraction_test.cpp`


### Modified
- `cmake/Dependencies.cmake`
- `cmake/Tests.cmake`


## Stats

- **5** files changed
- **690** insertions(+)
- **1** deletions(-)
