# [28] feat: add custom exception hierarchy and variable validation

**Commit**: `7e548da` ([`7e548dafa09382ff93751562d137746d5f707843`](https://github.com/licoded/CosyZeroRewrite/commit/7e548dafa09382ff93751562d137746d5f707843))
**Date**: 2026-01-02 02:46:55 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add FormulaException base class with parse_error, semantic_error, etc.
- Add variable name validation (regex pattern, max length, keyword blacklist)
- Update FormulaPool with enhanced error handling

## AI Analysis

### 📝 Change Summary
实现 add custom exception hierarchy and variable validation。

## Changes

### Added
- `include/formula/formula_exception.hpp`


### Modified
- `claude.md`
- `include/formula/formula_pool.hpp`
- `src/formula/formula_pool.cpp`


## Stats

- **4** files changed
- **358** insertions(+)
- **1** deletions(-)
