# [154] test: improve on_the_fly_synthesis_tests output format

**Commit**: `4beffe8` ([`4beffe83bbf95dbbbf67fdd774324e7a29b2e3ff`](https://github.com/licoded/CosyZeroRewrite/commit/4beffe83bbf95dbbbf67fdd774324e7a29b2e3ff))
**Date**: 2026-01-03 23:42:09 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add TEST_START/TEST_PASS/TEST_SKIP macros for better output
- Show test case name + formula instead of just file:line
- Add summary with Total/Passed/Skipped/Failed breakdown
- Update remaining test functions to use new format

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
- `tests/on_the_fly_synthesis_tests.cpp`


## Stats

- **1** files changed
- **81** insertions(+)
- **29** deletions(-)
