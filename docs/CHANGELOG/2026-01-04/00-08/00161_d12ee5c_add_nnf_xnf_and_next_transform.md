# [161] test: add NNF, XNF, and Next transformation tests

**Commit**: `d12ee5c` ([`d12ee5ca70d342ff7a24db46ab5cfe1c210a2370`](https://github.com/licoded/CosyZeroRewrite/commit/d12ee5ca70d342ff7a24db46ab5cfe1c210a2370))
**Date**: 2026-01-04 00:17:12 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add three separate test files for formula transformations:

1. nnf_tests.cpp (14 test cases)
   - Base cases: true, false, literal unchanged
   - Double negation: !!p → p
   - De Morgan's laws: !(p ∧ q) → !p ∨ !q
   - Temporal duality: !(p U q) → (!p) R (!q)
   - LTLf Next negation: !X(p) → X(!p) ∨ End
   - Idempotence check

2. xnf_tests.cpp (16 test cases)
   - Base cases: literals unchanged
   - Next operator: X(p) → X(p)
   - Until: p U q → q ∨ (p ∧ X(p U q))
   - Release: p R q → q ∧ (p ∨ X(p R q))
   - Nested temporal operators
   - Re-application behavior

3. next_tests.cpp (23 test cases)
   - Basic creation and properties
   - Complex formulas with Next
   - NNF interaction: X(p) → X(p), !X(p) → X(!p) ∨ End
   - XNF interaction: X(p U q) transforms child
   - Parsing from strings
   - Hash consistency and equality

All tests use Catch2 with compact default output and
verbose mode via COSY_TEST_VERBOSE=1.

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
- `tests/next_tests.cpp`
- `tests/nnf_tests.cpp`
- `tests/xnf_tests.cpp`


### Modified
- `cmake/Tests.cmake`


## Stats

- **4** files changed
- **1037** insertions(+)
- **1** deletions(-)
