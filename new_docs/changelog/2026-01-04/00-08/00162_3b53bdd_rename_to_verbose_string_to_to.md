# [162] refactor: rename to_verbose_string to to_string_with_names

**Commit**: `3b53bdd` ([`3b53bdda89c59fe1ed52d544eb1a91ef073ed6f6`](https://github.com/licoded/CosyZeroRewrite/commit/3b53bdda89c59fe1ed52d544eb1a91ef073ed6f6))
**Date**: 2026-01-04 00:41:48 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Rename method to clearer semantics: "string with variable names"
- Update all usages in tests (transformation_tests, nnf_fuzz_test, random_formula_test)
- Fuzz test now shows original variable names (p1, p2) instead of internal IDs (v0, v1)

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
- `tests/nnf_fuzz_test.cpp`


### Modified
- `cmake/Tests.cmake`
- `include/formula/formula.hpp`
- `src/formula/formula.cpp`
- `tests/nnf_tests.cpp`
- `tests/random_formula_test.cpp`
- `tests/transformation_tests.cpp`


## Stats

- **7** files changed
- **515** insertions(+)
- **24** deletions(-)
