# [195] fix: improve edge tooltip and hide GameGraph tooltip on blank areas

**Commit**: `a7d12ed` ([`a7d12eda782c17fa8063b18fd1ca84d2bce34499`](https://github.com/licoded/CosyZeroRewrite/commit/a7d12eda782c17fa8063b18fd1ca84d2bce34499))
**Date**: 2026-01-04 15:31:04 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

1. Filter out SVG root title ("GameGraph", "digraph") to prevent tooltip
   from appearing when hovering over blank areas

2. Always show Assignment row for edges, display "(none)" for env moves
   that don't have output assignments

Before:
- Sys move: showed Type and Assignment
- Env move: only showed Type (no Assignment row)
- Blank areas: showed "GameGraph" tooltip

After:
- Sys move: shows Type, Assignment, Formula (if available)
- Env move: shows Type and "Assignment: (none)"
- Blank areas: no tooltip shown

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
- **10** insertions(+)
- **2** deletions(-)
