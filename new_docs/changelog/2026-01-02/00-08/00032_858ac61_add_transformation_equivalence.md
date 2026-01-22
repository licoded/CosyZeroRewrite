# [32] feat: add transformation equivalence tests

**Commit**: `858ac61` ([`858ac618aefe9ee4d9e586d0320c0c83c37a68a5`](https://github.com/licoded/CosyZeroRewrite/commit/858ac618aefe9ee4d9e586d0320c0c83c37a68a5))
**Date**: 2026-01-02 03:09:10 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Test 1: Parse → to_verbose_string → Re-parse (49 cases)
- Test 2: NNF transformation equivalence (49 cases)
- Test 3: XNF transformation equivalence (49 cases)
- Test 4: Full pipeline equivalence (49 cases)
- All 196 tests pass
- Failed cases logged to logs/transform_failures_*.log with context
- Summary logged to logs/transform_summary_*.log

## AI Analysis

### 📝 Change Summary
实现 add transformation equivalence tests。

## Changes

### Added
- `tests/transformation_tests.cpp`


### Modified
- `cmake/Tests.cmake`


## Stats

- **2** files changed
- **570** insertions(+)
- **1** deletions(-)
