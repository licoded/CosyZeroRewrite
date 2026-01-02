# [48] test: add comprehensive TableauState unit tests

**Commit**: `7af902e` ([`7af902eb7f256899decd21146121b3d7610ca514`](https://github.com/anthropics/cosy-zero/commit/7af902eb7f256899decd21146121b3d7610ca514))
**Date**: 2026-01-02 09:52:20 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add 29 test cases (36 assertions) for TableauState accepting
conditions, validating the Until formula fix.

Key tests:
- Accepting state conditions (empty, true, false, literals)
- Until accepting states: THE FIX VALIDATION
  * Until with right side satisfied → accepting
  * Until WITHOUT right side → NOT accepting (key bug fix)
  * Eventually (F p) with/without p satisfied
  * Multiple Until formulas
- Local consistency (contradictions, And/Or rules)
- Release formula accepting states
- Next formula behavior
- next() state computation
- Hash consing (TableauStatePool)
- OnTheFlyDFA successor computation and caching
- Complex nested formulas

All 36 assertions passing.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `tests/tableau_state_test.cpp`


### Modified
- `cmake/Tests.cmake`


## Stats

- **2** files changed
- **538** insertions(+)
- **1** deletions(-)
