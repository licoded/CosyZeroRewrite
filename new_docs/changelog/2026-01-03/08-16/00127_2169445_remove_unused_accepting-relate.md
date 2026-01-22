# [127] refactor: remove unused accepting-related functions

**Commit**: `2169445` ([`216944570e4d877c482dac9982364c99d21aed24`](https://github.com/licoded/CosyZeroRewrite/commit/216944570e4d877c482dac9982364c99d21aed24))
**Date**: 2026-01-03 09:01:19 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Remove functions and members that are not used in current implementation:
- GameGraph::is_dfa_accepting() - unused function
- GameSolver::is_scc_accepting() - unused function
- GameGraph::dfa_ member - only used by is_dfa_accepting()
- GameGraph::dfa_to_game_ member - never read after initialization
- Remove unused <unordered_map> include

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


## Stats

- **2** files changed
- **1** insertions(+)
- **63** deletions(-)
