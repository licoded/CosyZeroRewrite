# [30] fix: correct stress test expectations

**Commit**: `a44cc2b` ([`a44cc2b7396f3e9f9e19dac04b295b170c02e9cf`](https://github.com/licoded/CosyZeroRewrite/commit/a44cc2b7396f3e9f9e19dac04b295b170c02e9cf))
**Date**: 2026-01-02 02:59:19 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Fix 'Neg: Double negation chain': 5 negations = !p1, not p1
- Use optional<bool> for expected_equiv to handle uncertain cases
- Random tests with different formulas don't have expectations
- Display INFO for tests without expectations instead of FAIL

## AI Analysis

### 📝 Change Summary
修复 correct stress test expectations。

## Changes

### Modified
- `tests/stress_test.cpp`


## Stats

- **1** files changed
- **32** insertions(+)
- **17** deletions(-)
