# [187] feat: add detailed state data to trace tooltips

**Commit**: `5c5a065` ([`5c5a0659a46086352777b56ba94fee80b85f389d`](https://github.com/licoded/CosyZeroRewrite/commit/5c5a0659a46086352777b56ba94fee80b85f389d))
**Date**: 2026-01-04 14:24:31 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add StateData struct with phi, xnf_phi, prop_atoms fields
- Modify TraceExporter to collect state data from solver
- Update trace JSON output to include state_data map
- Update frontend TypeScript types to include state_data
- Update GraphCanvas component to display detailed tooltips matching game_graph HTML format

The tooltip now shows:
- State ID with initial badge
- Classification badge (Swin/Ewin/Unknown with color)
- Type (System/Environment)
- Formula (phi)
- XNF Formula
- Propositional Atoms

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
- `include/synthesis/trace_exporter.hpp`
- `src/synthesis/trace_exporter.cpp`
- `visualization/src/App.vue`
- `visualization/src/components/GraphCanvas.vue`
- `visualization/src/types/trace.ts`


## Stats

- **5** files changed
- **308** insertions(+)
- **58** deletions(-)
