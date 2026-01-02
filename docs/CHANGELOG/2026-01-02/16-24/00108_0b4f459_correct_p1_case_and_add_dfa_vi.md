# [108] fix: correct !p1 case and add DFA visualization

**Commit**: `0b4f459` ([`0b4f4597a65671662128a657551c65748215700b`](https://github.com/licoded/CosyZeroRewrite/commit/0b4f4597a65671662128a657551c65748215700b))
**Date**: 2026-01-02 21:02:33 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

## Bug Fix: !p1 (p1 as system variable)

**Root Cause**: TableauState::initial() was expanding all subformulas
recursively, including the child of Not operators. This caused !p1 to
expand to {v0, !v0}, which is locally inconsistent (both p1 and !p1).

**Fix**: Don't expand child of Not operators in initial state.
- !v0 → {!v0} ✓ (correct)
- Before: !v0 → {v0, !v0} ✗ (inconsistent)

**Related Changes**:
1. Add DOT format visualization for game graph debugging
2. Mark empty DFA states as accepting (represents satisfied formula)
3. Don't add successor when next DFA state is terminal (empty)

## New Documentation

- docs/WORKFLOWS/debug_dfa_visualization.md - DFA visualization guide
- docs/ARCHITECTURE/tableau_initial_state_design.md - Design discussion

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
- `docs/ARCHITECTURE/tableau_initial_state_design.md`
- `docs/WORKFLOWS/debug_dfa_visualization.md`


### Modified
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **4** files changed
- **298** insertions(+)
- **10** deletions(-)
