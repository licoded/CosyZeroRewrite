# [191] feat: add partition info and formula display in edge tooltips

**Commit**: `1695c53` ([`1695c5369b61fcecf46463de728207289f6532c2`](https://github.com/licoded/CosyZeroRewrite/commit/1695c5369b61fcecf46463de728207289f6532c2))
**Date**: 2026-01-04 15:08:45 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add TracePartition struct to trace_exporter.hpp with inputs/outputs
- Extract partition information from FormulaPool in TraceExporter constructor
- Include partition in trace JSON output
- Update GraphCanvas.vue to:
  - Accept partition as prop
  - Parse assignment labels (e.g., "out={0}") to convert indices to variable names
  - Generate formulas from assignments (e.g., "{0}" with outputs=["p"] -> "p")
  - Display formatted assignment and formula in edge tooltips

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
- **144** insertions(+)
- **8** deletions(-)
