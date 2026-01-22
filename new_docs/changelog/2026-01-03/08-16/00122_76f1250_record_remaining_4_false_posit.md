# [122] docs: record remaining 4 False Positives for further investigation

**Commit**: `76f1250` ([`76f1250a897bdbbf4527e4e5483f692fb66c45e1`](https://github.com/licoded/CosyZeroRewrite/commit/76f1250a897bdbbf4527e4e5483f692fb66c45e1))
**Date**: 2026-01-03 08:13:30 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

This documents the 4 remaining False Positives after the recent
improvements to Release operator handling:

- f104: (p7) R (((!(p3)) R (X(!(p4)))) U (p0))
- f114: F((X(X(G(!(p8))))) R ((p3) & (F(p4)) & ((p6) U (G(p5)))))
- f115: (X(X(p3))) R ((G(p4)) U (p0))
- f129: F((F(G(p4))) & (G((p5) & ((p1) | (!(p5)))))))

All verified as UNREALIZABLE by Cosy reference implementation.

Current accuracy: 91.84% (45/49)

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

### Added
- `docs/working_issues/2026-01-03_00_AM_RemainingFalsePositives/BUG_REPORT.md`


## Stats

- **1** files changed
- **112** insertions(+)
- **0** deletions(-)
