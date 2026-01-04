# [241] feat: add --sleep parameter to benchmark_test for testing progress bar

**Commit**: `1e9352d` ([`1e9352dde5c1d21a13513f6994a4551b3173ca89`](https://github.com/licoded/CosyZeroRewrite/commit/1e9352dde5c1d21a13513f6994a4551b3173ca89))
**Date**: 2026-01-05 00:39:00 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add --sleep option to artificially slow down each task, allowing
observation of the progress bar behavior during parallel execution.

Changes:
- Add --sleep parameter (0-60 seconds) to CLI11 options
- Pass sleep_per_task to BenchmarkRunner and process_formula
- Use std::this_thread::sleep_for to delay after each parse
- Include <thread> header for sleep functionality

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
- **82** insertions(+)
- **48** deletions(-)
