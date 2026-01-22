# [210] fix: filter non-literal formulas from prop_atoms when extracting relevant variables

**Commit**: `84822fd` ([`84822fd5148d1a68cb319af45897ce7c58aeda0f`](https://github.com/licoded/CosyZeroRewrite/commit/84822fd5148d1a68cb319af45897ce7c58aeda0f))
**Date**: 2026-01-04 20:50:04 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

prop_atoms may contain non-literal formulas like X(...), |, &, etc.
These should be skipped when extracting relevant variables for move enumeration.

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
- **4** insertions(+)
- **3** deletions(-)
