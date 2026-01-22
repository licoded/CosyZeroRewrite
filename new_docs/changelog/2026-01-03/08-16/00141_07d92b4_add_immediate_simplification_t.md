# [141] fix: add immediate simplification to formula_progression

**Commit**: `07d92b4` ([`07d92b4b737fc94606fbf899794f1ce3fdb6f5f4`](https://github.com/licoded/CosyZeroRewrite/commit/07d92b4b737fc94606fbf899794f1ce3fdb6f5f4))
**Date**: 2026-01-03 11:04:31 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Prevents formula explosion by simplifying during progression:
- (true & false) → false, (false & anything) → false
- (true | anything) → true, (false | φ) → φ
- !!φ → φ, !true → false, !false → true

F (p1 & !p1) test now passes: Unrealizable (correct)

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


## Stats

- **1** files changed
- **33** insertions(+)
- **2** deletions(-)
