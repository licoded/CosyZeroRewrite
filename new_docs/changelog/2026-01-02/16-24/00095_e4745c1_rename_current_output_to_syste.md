# [95] refactor: rename current_output to system_chosen_output

**Commit**: `e4745c1` ([`e4745c10cbfbb5e13ba232c852cb5e0d489f40b6`](https://github.com/licoded/CosyZeroRewrite/commit/e4745c10cbfbb5e13ba232c852cb5e0d489f40b6))
**Date**: 2026-01-02 18:01:05 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

The variable name current_output was ambiguous. Renamed to
system_chosen_output to clearly indicate it's the output chosen
by System in the previous turn, used during Environment's turn.

Changes:
- GameState::system_chosen_output (was current_output)
- Updated all usages in solver and strategy modules
- Updated documentation

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
- `docs/ARCHITECTURE/components.md`
- `docs/ARCHITECTURE/synthesis.md`
- `include/synthesis/on_the_fly_solver.hpp`
- `src/synthesis/on_the_fly_solver.cpp`
- `src/synthesis/strategy.cpp`


## Stats

- **5** files changed
- **16** insertions(+)
- **16** deletions(-)
