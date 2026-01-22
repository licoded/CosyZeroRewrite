# [160] fix: use correct -d yes instead of -d 0 for Catch2 durations

**Commit**: `28cd624` ([`28cd6248049ec3fde2924101f05b43b8df9a01c2`](https://github.com/licoded/CosyZeroRewrite/commit/28cd6248049ec3fde2924101f05b43b8df9a01c2))
**Date**: 2026-01-04 00:06:09 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

The -d option in Catch2 v2 takes yes|no, not a number.
Correct usage: -d yes (shows duration like "0.000 s: Test name")

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
- **2** insertions(+)
- **2** deletions(-)
