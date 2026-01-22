# [232] fix: explicitly flush logger before exit to avoid hang

**Commit**: `e2dd2fb` ([`e2dd2fb88cbfea1a3129efece0d46d74a18a6fcf`](https://github.com/licoded/CosyZeroRewrite/commit/e2dd2fb88cbfea1a3129efece0d46d74a18a6fcf))
**Date**: 2026-01-04 23:44:40 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added explicit logger flush and shutdown in benchmark.cpp
to prevent hanging on program exit.

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
- **5** insertions(+)
- **0** deletions(-)
