# [135] debug: add test for eventually_contradiction case

**Commit**: `cba7b04` ([`cba7b04236b3de58d8a6f6838afe8d04b5b3df2d`](https://github.com/licoded/CosyZeroRewrite/commit/cba7b04236b3de58d8a6f6838afe8d04b5b3df2d))
**Date**: 2026-01-03 10:03:20 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added debug test program to analyze the F(p1 & !p1) case which
should be unrealizable but currently returns realizable.

Also fixed LOG_DEBUG pointer formatting issues (spdlog doesn't
allow formatting non-void pointers directly).

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
- `tests/debug_eventually_contradiction.cpp`


### Modified
- `cmake/Tests.cmake`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **3** files changed
- **56** insertions(+)
- **5** deletions(-)
