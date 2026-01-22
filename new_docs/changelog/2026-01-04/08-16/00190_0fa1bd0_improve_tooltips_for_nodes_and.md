# [190] fix: improve tooltips for nodes and edges

**Commit**: `0fa1bd0` ([`0fa1bd0bf5547235c0ace9c570521cb844b98325`](https://github.com/licoded/CosyZeroRewrite/commit/0fa1bd0bf5547235c0ace9c570521cb844b98325))
**Date**: 2026-01-04 14:56:16 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Fix "Unknown" badge showing when stateData is null (now only shows classification badge when stateData exists)
- Add edge tooltip support showing:
  - Edge direction (from → to)
  - Edge type (System move / Environment move)
  - Assignment label (e.g., out={0})

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
- `visualization/src/components/GraphCanvas.vue`


## Stats

- **1** files changed
- **130** insertions(+)
- **47** deletions(-)
