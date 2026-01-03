# [157] test: use compact reporter for cleaner default output

**Commit**: `ea94960` ([`ea949604fed820afb04cc49966f59bc7157ae002`](https://github.com/licoded/CosyZeroRewrite/commit/ea949604fed820afb04cc49966f59bc7157ae002))
**Date**: 2026-01-03 23:56:24 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Use Catch2's compact reporter in default mode
- Shows failures in one line per test
- Verbose mode (COSY_TEST_VERBOSE=1) still shows full details
- Suppress unused parameter warning

Default output example:
  file.cpp:49: failed: result for: false with 1 message: 'Formula: p1'

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
- **8** insertions(+)
- **3** deletions(-)
