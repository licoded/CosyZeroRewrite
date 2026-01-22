# [164] feat: add XNF fuzzing test

**Commit**: `3753064` ([`375306499fedf5d3b2cef7b12d47568829c756fc`](https://github.com/licoded/CosyZeroRewrite/commit/375306499fedf5d3b2cef7b12d47568829c756fc))
**Date**: 2026-01-04 00:47:46 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add xnf_fuzz_test.cpp with:
- XNF validation (NNF + U/R operators only inside X)
- Random formula generation with configurable depth/variables
- Support for printing first N formulas with XNF results
- 5000+ formulas tested with 100% pass rate

Usage: ./xnf_fuzz_test [num_formulas] [max_depth] [num_variables] [print_count]

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

### Added
- `tests/xnf_fuzz_test.cpp`


### Modified
- `cmake/Tests.cmake`


## Stats

- **2** files changed
- **384** insertions(+)
- **0** deletions(-)
