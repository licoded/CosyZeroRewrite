# [192] test: add XNF progression test for F p

**Commit**: `5fe7996` ([`5fe7996a89489ebf1ed028fa279cfe2454878045`](https://github.com/licoded/CosyZeroRewrite/commit/5fe7996a89489ebf1ed028fa279cfe2454878045))
**Date**: 2026-01-04 15:12:18 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added test case:
- FP: XNF(F p) with p ∈ sigma → true
- Tests XNF conversion of "true U p" and progression with p=true
- Verifies that XNF(F p) = p | X(F p) progresses to true when p is in sigma

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
- `tests/automata/progression.cpp`


## Stats

- **1** files changed
- **23** insertions(+)
- **0** deletions(-)
