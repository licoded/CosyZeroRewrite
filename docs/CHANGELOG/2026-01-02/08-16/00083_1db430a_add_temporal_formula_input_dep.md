# [83] fix(synthesis): add temporal formula input dependency checks and debug

**Commit**: `1db430a` ([`1db430a572e109c5dd5b098b9997c6a5540b0f3e`](https://github.com/licoded/CosyZeroRewrite/commit/1db430a572e109c5dd5b098b9997c6a5540b0f3e))
**Date**: 2026-01-02 14:26:13 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Partially addresses the 64% benchmark accuracy issue by identifying and
fixing specific problems with temporal formulas:

1. Added input dependency checks for temporal formulas:
   - Release (false R p): reject if p requires input to be true
   - Until (φ U ψ): reject if ψ requires input to be true
   - Next (X φ): reject if φ requires input to be true

2. Identified empty state problem:
   - When Next literal is not satisfied, state becomes empty
   - Empty state was accepting, allowing Environment to "win"
   - Added check to reject empty states for synthesis

3. Test results after fixes:
   - G(p5) where p5 is input: now correctly Unrealizable ✓
   - X(p5) where p5 is input: now correctly Unrealizable ✓
   - Simple test cases: all pass ✓
   - Small benchmark (50 formulas): 55% accuracy

4. Known issues:
   - Empty state rejection is too aggressive
   - Need to distinguish "good empty" (formula satisfied) vs "bad empty" (failed)
   - Some Until formulas may be incorrectly rejected

Related: #82 - docs/working_issues/2026-01-02_PM_AccuracyIssue/BUG_REPORT.md

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `docs/working_issues/2026-01-02_PM_AccuracyIssue/BUG_REPORT.md`
- `.gitignore`
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **4** files changed
- **141** insertions(+)
- **7** deletions(-)
