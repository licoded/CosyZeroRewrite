# [223] docs: record F/G operator bugs found during f13 analysis

**Commit**: `c488293` ([`c48829359ece37d256e89726bd4ac3d52127a38c`](https://github.com/licoded/CosyZeroRewrite/commit/c48829359ece37d256e89726bd4ac3d52127a38c))
**Date**: 2026-01-04 22:54:09 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

During analysis of the 4 FAIL cases (f101, f108, f117, f13),
found that XNF fix resolved f101/f108/f117, but discovered
new bugs with F/G operators.

Mismatch cases:
- X(!(p3)): Cosy2=UNREALIZABLE, Cosy=Realizable
- G(!(p3)): Cosy2=REALIZABLE, Cosy=Unrealizable
- G(p3): Cosy2=REALIZABLE, Cosy=Unrealizable
- X(F(!(p3))): Cosy2=UNREALIZABLE, Cosy=Realizable

Updated docs/BUGS/open.md with details and analysis directions.

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
- `docs/BUGS/open.md`


## Stats

- **1** files changed
- **44** insertions(+)
- **16** deletions(-)
