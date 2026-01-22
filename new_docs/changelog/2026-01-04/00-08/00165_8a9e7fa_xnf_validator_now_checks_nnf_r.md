# [165] fix: xnf validator now checks NNF rules inside X(...)

**Commit**: `8a9e7fa` ([`8a9e7fa514f438bdf9d122752d716c798d7440c0`](https://github.com/licoded/CosyZeroRewrite/commit/8a9e7fa514f438bdf9d122752d716c798d7440c0))
**Date**: 2026-01-04 00:51:21 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add check_nnf_rules() helper that:
- Checks NNF properties (no naked negations)
- But ALLOWS U/R operators (unlike strict is_nnf)

Previously, X(...) content was not validated, which could miss
NNF violations inside Next operators.

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
- `tests/xnf_fuzz_test.cpp`


## Stats

- **1** files changed
- **81** insertions(+)
- **7** deletions(-)
