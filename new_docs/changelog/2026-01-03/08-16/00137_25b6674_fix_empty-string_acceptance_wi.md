# [137] refactor: fix empty-string acceptance with recursive formula check

**Commit**: `25b6674` ([`25b6674d738c1a945dc4025bdb36fe9fb7547ea5`](https://github.com/licoded/CosyZeroRewrite/commit/25b6674d738c1a945dc4025bdb36fe9fb7547ea5))
**Date**: 2026-01-03 10:15:13 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Rewrote is_empty_string_accepting to use recursive formula evaluation:
- Checks formula structure (And/Or/Not) instead of just looking for U/X
- And: both sides must accept
- Or: at least one side accepts (handles (F a) | (G a) correctly)
- Not: accepts if child rejects
- Until/Next: never accept
- Release: accepts if right side accepts

This correctly handles cases like:
- F a (Until) → cannot be empty-string accepting
- (F a) | (G a) → CAN be accepting (Or with G a branch)

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
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **1** files changed
- **76** insertions(+)
- **57** deletions(-)
