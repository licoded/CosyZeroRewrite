# [144] refactor: reorganize logs and results directory structure

**Commit**: `2bc5062` ([`2bc5062a8a487bd7ea2d0029671b6f75c307f5f8`](https://github.com/licoded/CosyZeroRewrite/commit/2bc5062a8a487bd7ea2d0029671b6f75c307f5f8))
**Date**: 2026-01-03 18:44:19 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- New structure: YYYY-MM-DD/period/basename.{log,csv}
- Periods: 01-morning(6-12), 02-afternoon(12-18), 03-evening(18-24), 04-night(0-6)
- Updated src/automata/tableau.cpp for debug log
- Updated tools/benchmark_runner.cpp for logs and results

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
- `src/automata/tableau.cpp`
- `tools/benchmark_runner.cpp`


## Stats

- **2** files changed
- **53** insertions(+)
- **21** deletions(-)
