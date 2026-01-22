# [139] refactor: redesign TableauState with phi-based architecture

**Commit**: `8e65944` ([`8e65944f3b9a2090d9d8f140bde15e9c4456b298`](https://github.com/licoded/CosyZeroRewrite/commit/8e65944f3b9a2090d9d8f140bde15e9c4456b298))
**Date**: 2026-01-03 10:59:54 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- TableauState now stores phi_ (original), xnf_phi_ (for progression), prop_atoms_ (PA)
- Hash based only on phi_ for unique identification
- Implemented formula_progression(fp) per AAAI2019 Li et al.
- TableauState::next_phi() returns progressed formula for pool.get_or_create()
- Updated is_empty_string_accepting to use phi_ instead of formulas()
- Removed old is_accepting, is_locally_consistent methods

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
- `include/automata/tableau.hpp`
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`
- `tests/debug_eventually_contradiction.cpp`


## Stats

- **4** files changed
- **211** insertions(+)
- **1049** deletions(-)
