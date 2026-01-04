# [211] fix: handle unclassified s_prime in classify_scc

**Commit**: `5f6c4d5` ([`5f6c4d5d2d13c657a0ed046ceacd2a63c37ce104`](https://github.com/licoded/CosyZeroRewrite/commit/5f6c4d5d2d13c657a0ed046ceacd2a63c37ce104))
**Date**: 2026-01-04 20:54:14 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

When checking env moves, if s_prime is not yet classified, treat it as
not leading to Swin (all_env_moves_swin = false). This handles the case
where fixed-point iteration encounters states not yet classified.

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
- **1** deletions(-)
