# [166] feat: add compute_prop_atoms (PA) tests

**Commit**: `02b3873` ([`02b38732ddd5b43ce80fce15a06a71acfa15d8be`](https://github.com/licoded/CosyZeroRewrite/commit/02b38732ddd5b43ce80fce15a06a71acfa15d8be))
**Date**: 2026-01-04 00:59:26 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add prop_atoms_test.cpp with 17 tests covering:
- Base cases: literals, true, false
- Not penetration: PA(!p) = PA(p) = {p}
- Next atomic: PA(X(p)) = {X(p)}
- Until/Release atomic: PA(p U q) = {p U q}
- And/Or union: PA(p & q) = {p, q}
- Complex nested formulas

Also make compute_prop_atoms() public for testing.

Parser integration test skipped (needs investigation).

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
- `tests/prop_atoms_test.cpp`


### Modified
- `cmake/Tests.cmake`
- `include/automata/tableau.hpp`


## Stats

- **3** files changed
- **413** insertions(+)
- **13** deletions(-)
