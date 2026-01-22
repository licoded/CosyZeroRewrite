# [213] refactor: remove unnecessary local index conversion for input variables

**Commit**: `d3c1a97` ([`d3c1a97def1c9f38d935820c6300c2da76ce9adc`](https://github.com/licoded/CosyZeroRewrite/commit/d3c1a97def1c9f38d935820c6300c2da76ce9adc))
**Date**: 2026-01-04 21:06:24 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Store global indices directly in environment_chosen_input instead of
converting to local indices and back. This simplifies the code and
fixes the DOT label issue where env={p1} was showing as env={}.

Changes:
- expand_state: use global indices for relevant_input_var_ids
- expand_state: remove offset when building full assignment
- trace_exporter: use global indices directly for env edge labels

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
- `src/synthesis/on_the_fly_solver.cpp`
- `src/synthesis/trace_exporter.cpp`


## Stats

- **2** files changed
- **9** insertions(+)
- **16** deletions(-)
