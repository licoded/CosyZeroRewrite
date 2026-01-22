# [198] feat: use variable names in edge labels (sys={p} instead of out={0})

**Commit**: `c7c85f1` ([`c7c85f13d2e77dbc95f20c9fc808bf626d5c1999`](https://github.com/licoded/CosyZeroRewrite/commit/c7c85f13d2e77dbc95f20c9fc808bf626d5c1999))
**Date**: 2026-01-04 16:23:17 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Backend changes:
- Add `pool` pointer to StateIdMap for variable name lookup
- Replace `get_assignment_string()` with `get_assignment_label()`
- Output format: sys={p, q} or env={} instead of out={0,1} or in={}

Frontend changes:
- Update parseEdgeInfo() to detect sys=/env= prefix
- Simplify parseAssignment() to directly extract variable names
- assignmentToFormula() now works with variable names directly

This makes the graph visualization more user-friendly as users can
see actual variable names instead of indices.

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
- `src/synthesis/game_graph_export.cpp`
- `visualization/src/components/GraphCanvas.vue`


## Stats

- **2** files changed
- **83** insertions(+)
- **86** deletions(-)
