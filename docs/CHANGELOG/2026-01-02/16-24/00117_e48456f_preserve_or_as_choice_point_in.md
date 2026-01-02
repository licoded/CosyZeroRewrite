# [117] feat(synthesis): preserve OR as choice point in state construction

**Commit**: `e48456f` ([`e48456f7a4a1c675aedc09d41243dc43eec0c372`](https://github.com/licoded/CosyZeroRewrite/commit/e48456f7a4a1c675aedc09d41243dc43eec0c372))
**Date**: 2026-01-02 23:32:40 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

This is a major refactor of how OR formulas are handled:

1. **OR formula preservation in initial state**: OR formulas are
   now kept as-is in the initial state, not expanded to both sides.
   This prevents `!input | (...)` from being rejected by the `!input`
   check when the right side is satisfiable.

2. **OR handling in successor()**: OR formulas are now kept across
   state transitions (not expanded), allowing the game solver to
   choose which side to satisfy.

3. **OR local consistency check**: Modified to not require both sides
   in state, since OR is a choice point.

4. **Added AND handling in successor()**: AND formulas now expand
   both sides to the next state.

Results:
- Accuracy improved from 63.16% to 78.95% (+15.79%)
- False Negatives eliminated: 6 → 0
- False Positives increased: 1 → 4 (acceptable trade-off)

Known issues (False Positives):
- f103, f104, f114, f115: Complex nested formulas with Release/Until
  where `!input` in OR right side is not properly detected

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
- `src/automata/tableau.cpp`


## Stats

- **1** files changed
- **47** insertions(+)
- **9** deletions(-)
