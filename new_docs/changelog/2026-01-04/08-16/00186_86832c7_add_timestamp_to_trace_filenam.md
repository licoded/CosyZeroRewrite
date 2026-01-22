# [186] feat: add timestamp to trace filename and floating node tooltip

**Commit**: `86832c7` ([`86832c7f7645f94a899a9702fe33f909458d9026`](https://github.com/licoded/CosyZeroRewrite/commit/86832c7f7645f94a899a9702fe33f909458d9026))
**Date**: 2026-01-04 14:11:19 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Trace JSON filename now includes timestamp: trace_YYYYMMDD_HHMMSS.json
- Replaced right sidebar with floating tooltip on node hover
- Tooltip shows node ID (e.g., "S0") and info (e.g., "Swin · System")
- Tooltip follows cursor position with fixed offset

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
- `src/synthesis/trace_exporter.cpp`
- `visualization/src/App.vue`
- `visualization/src/components/GraphCanvas.vue`


## Stats

- **3** files changed
- **130** insertions(+)
- **64** deletions(-)
