# [171] test: replace next.cpp with formula_progression test

**Commit**: `58945ae` ([`58945ae80d88588efdfa246cfeeafeae0c718bf1`](https://github.com/licoded/CosyZeroRewrite/commit/58945ae80d88588efdfa246cfeeafeae0c718bf1))
**Date**: 2026-01-04 01:31:39 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Removed:
- tests/transformation/next.cpp (Next operator behavior tests)

Added:
- tests/automata/progression.cpp (Formula Progression tests)

The new test covers formula progression rules (AAAI2019):
- Base cases: true, false, literals
- Not: fp(!p, σ) = true if p ∉ σ
- Next: fp(X(φ), σ) = φ
- And/Or: Distribution over progression
- Complex formulas with X
- Parser integration tests

17 test cases, 21 assertions - all passing.

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
- `tests/automata/progression.cpp`


### Modified
- `cmake/Tests.cmake`


### Deleted
- `tests/transformation/next.cpp`


## Stats

- **3** files changed
- **385** insertions(+)
- **399** deletions(-)
