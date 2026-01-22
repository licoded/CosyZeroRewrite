# [142] docs: add working guidelines and debug tools for synthesis

**Commit**: `7ddd934` ([`7ddd934cc30540dedabc6b646ba847c1ae98978d`](https://github.com/licoded/CosyZeroRewrite/commit/7ddd934cc30540dedabc6b646ba847c1ae98978d))
**Date**: 2026-01-03 11:34:48 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- CLAUDE.md: add "先讨论再修改" requirement for debugging
- cmake/Tests.cmake: add debug_failing_tests target
- src/synthesis/on_the_fly_solver.cpp: add minimal debug logging for classification
- tests/debug_failing_tests.cpp: new debug test for on_the_fly_synthesis issues

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
- `tests/debug_failing_tests.cpp`


### Modified
- `CLAUDE.md`
- `cmake/Tests.cmake`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **4** files changed
- **137** insertions(+)
- **1** deletions(-)
