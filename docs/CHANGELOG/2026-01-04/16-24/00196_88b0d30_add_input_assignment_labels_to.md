# [196] feat: add input assignment labels to env move edges in game graph

**Commit**: `88b0d30` ([`88b0d307b65dcf53d92c8d03d3019794967cce35`](https://github.com/licoded/CosyZeroRewrite/commit/88b0d307b65dcf53d92c8d03d3019794967cce35))
**Date**: 2026-01-04 16:03:02 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Backend changes:
- Add environment_chosen_input field to GameState for tracking input assignments
- Update system_state() helper to accept input assignment parameter
- Store input assignment when creating system states from environment moves
- Export env move edges with "in={...}" labels in DOT output

Frontend changes (GraphCanvas.vue):
- Add "Classification:" label before classification badge
- Change tooltip layout from vertical to inline "key: value" format
- Left-align tooltip content
- Set max-width 360px for formula text with word-wrap
- Filter out "GameGraph" title to prevent blank area tooltip

Design note: environment_chosen_input is excluded from hash/equality
comparison to ensure system states with same DFA state are treated
as identical regardless of which input led to them.

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
- `src/synthesis/game_graph_export.cpp`
- `src/synthesis/on_the_fly_solver.cpp`
- `visualization/src/components/GraphCanvas.vue`


## Stats

- **4** files changed
- **63** insertions(+)
- **19** deletions(-)
