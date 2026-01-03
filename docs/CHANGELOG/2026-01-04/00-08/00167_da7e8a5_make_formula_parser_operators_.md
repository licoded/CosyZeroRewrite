# [167] fix: make formula parser operators case-sensitive (uppercase only)

**Commit**: `da7e8a5` ([`da7e8a53cc0184c66769f3caed2f7bc716698f73`](https://github.com/licoded/CosyZeroRewrite/commit/da7e8a53cc0184c66769f3caed2f7bc716698f73))
**Date**: 2026-01-04 01:06:30 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Only uppercase X, U, R, F, G are now recognized as LTL operators.
Lowercase x, u, r, f, g can be used as variable names.

Changes:
- Removed lowercase 'x' and 'u' from single-char operator cases
- Removed lowercase 'r', 'f', 'g' from ambiguous operator checks
- Made F/G keyword matching case-sensitive (removed tolower)
- Added parser integration tests for case sensitivity

Fixes prop_atoms_test parser integration tests.

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
- `src/formula/formula_parser.cpp`
- `tests/prop_atoms_test.cpp`


## Stats

- **2** files changed
- **69** insertions(+)
- **22** deletions(-)
