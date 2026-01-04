# [202] fix: use shared StateIdMap for consistent state IDs between DOT and state_data

**Commit**: `a7f2df9` ([`a7f2df9bbf6426bf402a67a255662190e039cba5`](https://github.com/licoded/CosyZeroRewrite/commit/a7f2df9bbf6426bf402a67a255662190e039cba5))
**Date**: 2026-01-04 17:47:04 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Root cause: to_dot() created a local StateIdMap on each call, while
TraceExporter used a persistent id_map_. This caused ID mismatches where
different GameState objects got the same ID.

Solution:
1. Move StateIdMap definition to public header (on_the_fly_solver.hpp)
2. Modify to_dot() to accept external StateIdMap parameter
3. TraceExporter now passes &id_map_ to solver.to_dot(&id_map_)
4. Both DOT and state_data now use the same StateIdMap instance

Fixes: E3 showing different classifications in DOT (Ewin) vs state_data (Swin)

Related: docs/working_issues/2026-01-04_PM_E3ClassificationBug/BUG_REPORT.md

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
- `docs/working_issues/2026-01-04_PM_E3ClassificationBug/BUG_REPORT.md`


### Modified
- `include/synthesis/on_the_fly_solver.hpp`
- `include/synthesis/trace_exporter.hpp`
- `src/synthesis/game_graph_export.cpp`
- `src/synthesis/trace_exporter.cpp`


## Stats

- **5** files changed
- **204** insertions(+)
- **130** deletions(-)
