# [128] refactor: remove unused parameters and simplify code

**Commit**: `ca65b06` ([`ca65b06723737c09bf791c589595929ba8f3dd43`](https://github.com/licoded/CosyZeroRewrite/commit/ca65b06723737c09bf791c589595929ba8f3dd43))
**Date**: 2026-01-03 09:03:06 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Remove unused parameters and simplify implementation:
- GameGraph constructor: remove output_vars, input_vars, pool params
- GameSolver::is_realizable: remove output_vars, input_vars params
- Simplify propagate_status: remove unused variables (scc_set, swin, etc.)
- Update test files to match new signatures

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

### Modified
- `include/synthesis/game_solver.hpp`
- `src/synthesis/game_solver.cpp`
- `src/synthesis/synthesis.cpp`
- `tests/synthesis_test.cpp`


## Stats

- **4** files changed
- **14** insertions(+)
- **67** deletions(-)
