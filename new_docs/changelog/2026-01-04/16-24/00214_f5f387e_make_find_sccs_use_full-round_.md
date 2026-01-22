# [214] fix: make find_sccs use full-round transitions (sys move + env move)

**Commit**: `f5f387e` ([`f5f387e40a625388b0490f19a9ba0ac2b7a7989c`](https://github.com/licoded/CosyZeroRewrite/commit/f5f387e40a625388b0490f19a9ba0ac2b7a7989c))
**Date**: 2026-01-04 21:18:45 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added new function get_full_round_successors() that returns System states
reachable through a complete sys+env move (sys_state → env → sys_state).

Modified find_sccs() to:
- Only process System states for SCC decomposition
- Use get_full_round_successors() instead of get_successors()

This aligns SCC decomposition with classify_scc(), which also only
considers complete sys+env moves as edges.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
修复 `find_sccs` 函数的 bug，使其只考虑完整的 sys+env move 作为 SCC 图的边。

### 🔍 Technical Details
- 新增 `get_full_round_successors(sys_state)` 函数：返回从 System 状态经过完整回合（sys move → env move）后到达的所有 System 状态
- 修改 `find_sccs`：只处理 System 状态，使用 `get_full_round_successors` 而不是 `get_successors`
- 与 `classify_scc` 保持一致：两者都只考虑完整的 sys+env move

### 📊 Impact Analysis
- SCC 分解现在只基于 System 状态之间的完整回合转移
- SCC 中不再包含 Environment 状态
- 修复了 SCC 分解与分类逻辑不匹配的问题

## Changes

### Modified
- `include/synthesis/on_the_fly_solver.hpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **2** files changed
- **55** insertions(+)
- **4** deletions(-)
