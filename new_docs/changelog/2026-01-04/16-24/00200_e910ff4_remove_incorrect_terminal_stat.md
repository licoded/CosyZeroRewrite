# [200] fix: remove incorrect terminal state check for empty prop_atoms

**Commit**: `e910ff4` ([`e910ff4e42c537369632f739096fc6ea308202c4`](https://github.com/licoded/CosyZeroRewrite/commit/e910ff4e42c537369632f739096fc6ea308202c4))
**Date**: 2026-01-04 16:57:56 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Root cause: The code incorrectly treated `formulas().empty()` (i.e.,
`prop_atoms_.empty()`) as a forced termination condition.

Understanding:
- Reaching `true` is NOT forced termination in LTLf
- Only explicit `End` marker forces termination
- Transitions TO true and FROM true must both be recorded

Changes:
- Removed `if (next_dfa->formulas().empty())` check
- Now all environment moves are recorded, even when next state is true
- E1 now correctly generates env move to S1 (phi=true state)

Fixes: E1 missing env move in (true U p) trace

Related: docs/working_issues/2026-01-04_PM_LTfTermination/BUG_REPORT.md

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
- **6** insertions(+)
- **12** deletions(-)
