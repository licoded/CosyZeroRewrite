# [234] feat: enhance benchmark output and support both bench1/bench2 directories

**Commit**: `7a97015` ([`7a970152b76bf337f95ec4cc0e1fa913a0276e61`](https://github.com/licoded/CosyZeroRewrite/commit/7a970152b76bf337f95ec4cc0e1fa913a0276e61))
**Date**: 2026-01-04 23:53:11 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

### Changes
- Added bench directory info (bench1/bench2) to output
- Display complete formula and partition (inputs/outputs)
- Added read_benchmark_from_dir() to specify bench directory
- Modified benchmark runner to iterate over bench1 and bench2
- New command line format: ./benchmark_test <dir> [all|1|2] [start] [end]

### Usage
- `./benchmark_test benchmarks/sm1000 all 1 10` - run f1-f10 from both bench1 and bench2
- `./benchmark_test benchmarks/sm1000 1 1 10` - run f1-f10 from bench1 only
- `./benchmark_test benchmarks/sm1000 2 1 10` - run f1-f10 from bench2 only

### Test Results (all 1000 formulas)
- Parsed: 689 (68.9%)
- Failed parse: 311 (31.1% - mostly due to `->` implies operator)
- Total time: 17.4ms

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
- `include/synthesis/synthesis.hpp`
- `src/synthesis/synthesis.cpp`
- `tests/bench/benchmark.cpp`


## Stats

- **3** files changed
- **125** insertions(+)
- **23** deletions(-)
