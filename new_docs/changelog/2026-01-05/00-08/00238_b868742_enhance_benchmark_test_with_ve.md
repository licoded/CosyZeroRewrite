# [238] feat: enhance benchmark_test with verbosity options

**Commit**: `b868742` ([`b8687425779543e5cb00655023d89f2392e0e0b1`](https://github.com/licoded/CosyZeroRewrite/commit/b8687425779543e5cb00655023d89f2392e0e0b1))
**Date**: 2026-01-05 00:08:27 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added command-line options to control output verbosity:
- -v, --verbose: Print all cases with full details
- -q, --quiet: Only print summary (no per-case output)
- -p, --progress: Enable progress indicators (default)
- --no-progress: Disable progress output
- -h, --help: Show help message

Default behavior (Normal mode):
- Only shows failed cases with full details
- Shows progress every 100 formulas
- Always prints summary with colored status

Usage examples:
  benchmark_test                    # Normal mode, all 1000 formulas
  benchmark_test -v                 # Verbose mode, full output
  benchmark_test -q all 1 10        # Quiet mode, only summary
  benchmark_test --no-progress      # No progress indicators

Benefits:
- Reduced output volume by ~99% in default mode
- Cleaner console output for large test runs
- Easier to spot failures with colored status
- Flexible output control for different use cases

Modified files:
- tests/bench/benchmark.cpp: Added argument parsing and verbosity modes

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
- **209** insertions(+)
- **79** deletions(-)
