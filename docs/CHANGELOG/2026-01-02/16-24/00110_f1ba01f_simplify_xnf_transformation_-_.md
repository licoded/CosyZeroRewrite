# [110] refactor: simplify XNF transformation - remove F(true)/G(false)

**Commit**: `f1ba01f` ([`f1ba01fc700f932d2071375e99c4625038a8aeb0`](https://github.com/licoded/CosyZeroRewrite/commit/f1ba01fc700f932d2071375e99c4625038a8aeb0))
**Date**: 2026-01-02 21:38:06 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Until: xnf(φ₁ U φ₂) = xnf(φ₂) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))
  - No ¬End constraint needed, X implicitly enforces !End
- Release: xnf(φ₁ R φ₂) = (xnf(φ₂) ∨ End) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))
  - End marker allows termination (WX semantics)
- Updated documentation to use End marker instead of F/G

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
- `src/formula/xnf.cpp`


## Stats

- **3** files changed
- **90** insertions(+)
- **60** deletions(-)
