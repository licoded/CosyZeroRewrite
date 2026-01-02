# [37] feat: implement tableau-based DFA construction

**Commit**: `13369b6` ([`13369b66541a989d2e59bbcf8c4acb463f39fc80`](https://github.com/anthropics/cosy-zero/commit/13369b66541a989d2e59bbcf8c4acb463f39fc80))
**Date**: 2026-01-02 09:13:54 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implement on-the-fly DFA construction using tableau states as sets
of subformulas, following Algorithm 1 from arXiv:2408.07324.

- TableauState: DFA state as set of formulas with hash consing
- TableauStatePool: State deduplication and memory management
- OnTheFlyDFA: Lazy transition computation with caching
- AssignmentGenerator: Enumerate all variable assignments

Key features:
- Local consistency checking (contradiction detection)
- Proper handling of Release formulas (keep both right side and formula)
- Until formula continuation when right side not satisfied
- Next formula unwrapping

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `include/automata/dfa.hpp`
- `include/automata/tableau.hpp`
- `src/automata/dfa.cpp`
- `src/automata/tableau.cpp`


## Stats

- **4** files changed
- **1804** insertions(+)
- **0** deletions(-)
