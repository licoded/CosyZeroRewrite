# [111] docs: clarify XNF transformation - use empty string semantics

**Commit**: `af2764f` ([`af2764f4d4a279f15105f1d894096c93e451feca`](https://github.com/licoded/CosyZeroRewrite/commit/af2764f4d4a279f15105f1d894096c93e451feca))
**Date**: 2026-01-02 21:42:10 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Key concepts confirmed:
- Until: cannot accept empty string (X implicitly !End)
- Release: can accept empty string when φ₂ is satisfied
- No explicit End marker needed in XNF transformation
- Empty string acceptance is checked during transition generation

Transformation rules:
- xnf(φ₁ U φ₂) = xnf(φ₂) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))
- xnf(φ₁ R φ₂) = xnf(φ₂) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))

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
- `docs/ARCHITECTURE/xnf_detailed.md`
- `docs/ARCHITECTURE/xnf_transformation.md`


## Stats

- **2** files changed
- **39** insertions(+)
- **24** deletions(-)
