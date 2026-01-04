# [184] fix: use result.output instead of result.src for @viz-js/viz

**Commit**: `7d7c843` ([`7d7c843f05dd290ac12ebf169936a84d6fd5bbe4`](https://github.com/licoded/CosyZeroRewrite/commit/7d7c843f05dd290ac12ebf169936a84d6fd5bbe4))
**Date**: 2026-01-04 13:44:49 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

The render() method returns RenderResult with structure:
{ status: "success", output: string, errors: [] }

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
- `visualization/src/composables/useGraphViz.ts`


## Stats

- **1** files changed
- **9** insertions(+)
- **2** deletions(-)
