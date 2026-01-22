# [158] test: allow -h/--help to show Catch2 usage

**Commit**: `7c1e48b` ([`7c1e48b861876282ecbb7bcfdbfd490f220c7c32`](https://github.com/licoded/CosyZeroRewrite/commit/7c1e48b861876282ecbb7bcfdbfd490f220c7c32))
**Date**: 2026-01-03 23:58:14 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Check for help options before applying default args
- Support -h, --help, -?, --list-reporters
- Users can now see all available Catch2 options

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
- **11** insertions(+)
- **2** deletions(-)
