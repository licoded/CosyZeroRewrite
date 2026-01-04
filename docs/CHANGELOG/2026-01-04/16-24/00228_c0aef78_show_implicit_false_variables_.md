# [228] feat: show implicit false variables in edge labels

**Commit**: `c0aef78` ([`c0aef78a956f8274a0a0df1b24ef583ff02f4496`](https://github.com/licoded/CosyZeroRewrite/commit/c0aef78a956f8274a0a0df1b24ef583ff02f4496))
**Date**: 2026-01-04 23:43:00 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Backend now shows all relevant variables in edge labels:
- sys={p1, !p5} instead of sys={p1}
- env={!p2, p3} instead of env={p3}

Modified:
- on_the_fly_solver.hpp: get_assignment_label() takes prop_atoms parameter
- game_graph_export.cpp: pass prop_atoms when generating labels
- trace_exporter.hpp/cpp: new format_assignment_label() method

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
- `include/synthesis/trace_exporter.hpp`
- `src/synthesis/game_graph_export.cpp`
- `src/synthesis/trace_exporter.cpp`


## Stats

- **4** files changed
- **138** insertions(+)
- **64** deletions(-)
