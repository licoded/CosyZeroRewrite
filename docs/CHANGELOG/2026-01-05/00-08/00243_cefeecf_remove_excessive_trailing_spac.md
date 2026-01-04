# [243] fix: remove excessive trailing spaces in progress bar

**Commit**: `cefeecf` ([`cefeecfbc89b8857382009a09eba96c362a0a765`](https://github.com/licoded/CosyZeroRewrite/commit/cefeecfbc89b8857382009a09eba96c362a0a765))
**Date**: 2026-01-05 00:41:14 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Track the previous output length and only add spaces needed to clear
the previous longer output, instead of always padding to a fixed
maximum width. This makes the progress bar display cleaner.

Changes:
- Add last_output_len_ member to track previous output length
- Only pad with spaces when current output is shorter than previous
- Remove the bar_width + 80 fixed padding

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
- `tests/bench/benchmark.cpp`


## Stats

- **1** files changed
- **10** insertions(+)
- **7** deletions(-)
