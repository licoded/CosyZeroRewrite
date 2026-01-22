# [131] feat: add nested env move expansion in OnTheFlyGameSolver

**Commit**: `0a79a32` ([`0a79a3216cf1dfc003ad0b00c5c8c4a5d5444f3b`](https://github.com/licoded/CosyZeroRewrite/commit/0a79a3216cf1dfc003ad0b00c5c8c4a5d5444f3b))
**Date**: 2026-01-03 09:42:38 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Implement nested env move check for Unknown successors in propagate_classification
- For System states: check if all env moves from a successor are Swin
- For System states: check if exists env move to Ewin for all successors
- Synchronize logic with GameSolver::propagate_status

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
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **1** files changed
- **77** insertions(+)
- **26** deletions(-)
