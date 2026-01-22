# [222] fix: X(φ) should be base case in XNF transformation

**Commit**: `115e1dd` ([`115e1ddd435a9d33cfd1ffa7053fbc78f3652242`](https://github.com/licoded/CosyZeroRewrite/commit/115e1ddd435a9d33cfd1ffa7053fbc78f3652242))
**Date**: 2026-01-04 22:43:04 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

According to docs/ARCHITECTURE/xnf_detailed.md, X(φ) (◦-formula) is
already in XNF and should NOT recurse into its child formula.

Changes:
- Modified xnf_with_tail() to treat is_next() as base case
- X(p0 R p2) now stays as X(p0 R p2) instead of expanding
- Updated tests to reflect correct behavior
- Enhanced documentation to emphasize this rule

XNF Base Cases (updated):
- literal, true, false, end, not: return as-is
- X(φ): return X(φ) WITHOUT recursing into φ (◦-formula)

Test results:
- All 17 XNF tests pass (44 assertions)
- All 11 unit test suites pass

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
- `src/formula/xnf.cpp`
- `tests/transformation/xnf.cpp`


## Stats

- **3** files changed
- **33** insertions(+)
- **12** deletions(-)
