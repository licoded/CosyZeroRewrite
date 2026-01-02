# [34] fix: correct to_verbose_string output format

**Commit**: `314d6cb` ([`314d6cb4229541c065b7ec4b43da2860e69e4931`](https://github.com/anthropics/cosy-zero/commit/314d6cb4229541c065b7ec4b43da2860e69e4931))
**Date**: 2026-01-02 03:20:23 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Change True/False to true/false for parse compatibility
- Add random formula test with 10000 generated formulas
- All tests pass (100% no crashes, no failures)

## Changes

### Added
- `tests/random_formula_test.cpp`


### Modified
- `src/formula/formula.cpp`


## Stats

- **2** files changed
- **319** insertions(+)
- **2** deletions(-)
