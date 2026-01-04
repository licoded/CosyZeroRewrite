# [194] fix: extract edge label from SVG text element for tooltip

**Commit**: `e02ebf2` ([`e02ebf2983f49a93aaa1c287083fc2447601ac44`](https://github.com/licoded/CosyZeroRewrite/commit/e02ebf2983f49a93aaa1c287083fc2447601ac44))
**Date**: 2026-01-04 15:25:31 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

The issue was that GraphViz stores edge labels in a separate <text>
element within the edge group, not in the <title> element.

Changes:
- Update handleMouseMove to query both title and text elements
- Modify parseEdgeInfo to accept labelText as second parameter
- Edge tooltip now correctly shows Assignment and Formula

Before: Only showed "Type: System/Environment move"
After: Shows "Assignment: out={p}" and "Formula: p"

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
- **23** insertions(+)
- **14** deletions(-)
