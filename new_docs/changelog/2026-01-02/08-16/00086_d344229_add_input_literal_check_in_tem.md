# [86] fix: add input literal check in temporal states and fix partition parsing

**Commit**: `d344229` ([`d344229e07d1cda91670b0a3a232f7690484a625`](https://github.com/licoded/CosyZeroRewrite/commit/d344229e07d1cda91670b0a3a232f7690484a625))
**Date**: 2026-01-02 14:59:48 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Two key fixes for synthesis accuracy:

1. OnTheFlyDFA::is_accepting() - Added input literal check in temporal states
   - Even temporal states with input literals cannot be accepting
   - Fixes cases like (p5) & (F(p8)) where p5 is input

2. Cosy2 - Fixed partition file parsing
   - Support ".outputs: var" format (variables on same line)
   - Previously only supported separate header and variables

Test results:
- f112: (p5) & (F(p8)) now correctly UNREALIZABLE ✓
- f104, f115, f118, f129 also fixed ✓

Known issue: Overall accuracy still low (56.25%)
- Fix may be too conservative
- More investigation needed on when to reject input literals

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
- `src/automata/tableau.cpp`
- `src/cosy2.cpp`


## Stats

- **2** files changed
- **67** insertions(+)
- **10** deletions(-)
