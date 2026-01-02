# [47] fix: correct accepting state condition for Until formulas

**Commit**: `97e2f8f` ([`97e2f8f26545b124f30e11a6654cef2bc009d8c0`](https://github.com/licoded/CosyZeroRewrite/commit/97e2f8f26545b124f30e11a6654cef2bc009d8c0))
**Date**: 2026-01-02 09:46:22 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Issue: is_accepting() was not properly checking that all Until
formulas have their right side satisfied.

Previous logic:
  if (right not in Γ AND left not in Γ) → false

Correct logic:
  if (right not in Γ) → false (regardless of left)

Reasoning:
  In LTLf, ψ1 U ψ2 means "ψ1 holds until ψ2 becomes true, AND
  ψ2 must eventually become true". If the trajectory ends
  with ψ1 U ψ2 still in state but ψ2 not satisfied, the
  Until is incomplete and the state cannot be accepting.

Also updated documentation to clarify the three accepting conditions:
1. false ∉ Γ
2. Γ is locally consistent
3. All Until formulas have right side satisfied

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
修复 correct accepting state condition for Until formulas。

## Changes

### Modified
- `migrationDocs/on_the_fly_synthesis/TABLEAU_DFA.md`
- `src/automata/tableau.cpp`


## Stats

- **2** files changed
- **13** insertions(+)
- **14** deletions(-)
