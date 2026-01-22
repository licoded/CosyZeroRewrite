# [149] fix: use correct relative paths for logs and results

**Commit**: `938cb35` ([`938cb35e86753a2f91b003115d4b4bec9ea3397f`](https://github.com/licoded/CosyZeroRewrite/commit/938cb35e86753a2f91b003115d4b4bec9ea3397f))
**Date**: 2026-01-03 19:12:38 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Changed from ../logs/ to logs/ because executables are run from
project root directory (e.g., ./build/benchmark_runner).

All logs now correctly go to:
- logs/formula/YYYY-MM-DD/period/formula_*.log
- logs/tableau/YYYY-MM-DD/period/tableau_debug_*.log
- logs/benchmark/YYYY-MM-DD/period/benchmark_*.log
- results/benchmark/YYYY-MM-DD/period/benchmark_results_*.csv

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
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **2** insertions(+)
- **2** deletions(-)
