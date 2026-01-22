# [113] feat: implement XNF transformation and integrate into OnTheFlyDFA

**Commit**: `bda737e` ([`bda737e44085b27a5a4099c561330a623d7bd19e`](https://github.com/licoded/CosyZeroRewrite/commit/bda737e44085b27a5a4099c561330a623d7bd19e))
**Date**: 2026-01-02 21:50:17 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

XNF Transformation (xnf_with_tail):
- Until: xnf(φ₁ U φ₂) = xnf(φ₂) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))
- Release: xnf(φ₁ R φ₂) = xnf(φ₂) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))
- No explicit End marker - empty string acceptance is implicit

Integration:
- OnTheFlyDFA now applies XNF transformation after NNF
- Initial state constructed from XNF formula

Bug Fixes:
- is_accepting: Check for Release with false inside Next
- is_locally_consistent: Check for Release with false inside Next
- Fixes G p1 test case (false R p1 semantics)

All tests passing (10/10).

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
- `src/formula/xnf.cpp`


## Stats

- **2** files changed
- **47** insertions(+)
- **18** deletions(-)
