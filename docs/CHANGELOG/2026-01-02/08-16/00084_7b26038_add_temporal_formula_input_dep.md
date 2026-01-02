# [84] fix(synthesis): add temporal formula input dependency checks

**Commit**: `7b26038` ([`7b260389c4a6a93879687044012d1c92e275d25a`](https://github.com/licoded/CosyZeroRewrite/commit/7b260389c4a6a93879687044012d1c92e275d25a))
**Date**: 2026-01-02 14:44:35 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

This commit fixes several critical issues in the LTLf synthesis algorithm
that were causing benchmark accuracy to be only 64% instead of ~100%.

Changes:
1. Added Next formula input dependency check in is_accepting()
   - X(input) is now correctly UNREALIZABLE
2. Added Release formula input dependency check in is_accepting()
   - G(input) is now correctly UNREALIZABLE
3. Fixed terminal state classification to distinguish System vs Environment turn
4. Added handling for failed input literals in successor()
5. Added friend declaration for OnTheFlyDFA to access TableauState::formulas_

Test results (simple tests):
- X(p6) output: Realizable ✓
- X(p5) input: Unrealizable ✓ (was failing)
- G(p5) input: Unrealizable ✓ (was failing)
- G(p6) output: Realizable ✓

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `docs/working_issues/2026-01-02_PM_AccuracyIssue/CONTINUATION.md`


### Modified
- `CLAUDE.md`
- `docs/BUGS/open.md`
- `docs/working_issues/2026-01-02_PM_AccuracyIssue/BUG_REPORT.md`
- `include/automata/tableau.hpp`
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **7** files changed
- **393** insertions(+)
- **52** deletions(-)
