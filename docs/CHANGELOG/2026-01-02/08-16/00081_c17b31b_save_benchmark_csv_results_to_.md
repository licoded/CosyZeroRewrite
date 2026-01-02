# [81] feat: save benchmark CSV results to organized results/ directory

**Commit**: `c17b31b` ([`c17b31b5bd93f50644562297ce8c3713069caa4e`](https://github.com/licoded/CosyZeroRewrite/commit/c17b31b5bd93f50644562297ce8c3713069caa4e))
**Date**: 2026-01-02 14:02:21 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Create results/benchmark/YYYY-MM-DD/HH-MM/ for CSV files
- Keep logs/benchmark/YYYY-MM-DD/HH-MM/ for log files
- Also save benchmark_results_latest.csv in build for convenience
- Results and logs are now in separate directories

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **31** insertions(+)
- **4** deletions(-)
