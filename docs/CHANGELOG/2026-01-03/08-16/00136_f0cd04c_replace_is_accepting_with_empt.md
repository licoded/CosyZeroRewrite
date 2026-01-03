# [136] refactor: replace is_accepting with empty-string acceptance check

**Commit**: `f0cd04c` ([`f0cd04ca32d9aff135f71967149d88d516da2c01`](https://github.com/licoded/CosyZeroRewrite/commit/f0cd04ca32d9aff135f71967149d88d516da2c01))
**Date**: 2026-01-03 10:11:37 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Changed the seed initialization in classify_scc:
- Old: DFA accepting states → Swin (incorrect for F(p1 & !p1))
- New: Empty-string accepting states → Swin

Empty-string acceptance rules (LTLf semantics):
- NO: U (Until), X (Next) - require future states
- YES: R (Release) and non-temporal formulas
- Also checks for contradictions (p & !p)

This fixes the F(p1 & !p1) case where the contradiction
was incorrectly marked as accepting.

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
- `include/synthesis/on_the_fly_solver.hpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **2** files changed
- **95** insertions(+)
- **5** deletions(-)
