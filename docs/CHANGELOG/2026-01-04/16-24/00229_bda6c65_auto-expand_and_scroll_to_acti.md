# [229] feat: auto-expand and scroll to active step in sidebar

**Commit**: `bda6c65` ([`bda6c65e8c6ede523d228fc49e5c018be562ad5d`](https://github.com/licoded/CosyZeroRewrite/commit/bda6c65e8c6ede523d228fc49e5c018be562ad5d))
**Date**: 2026-01-04 23:43:09 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

When a step is selected:
- Auto-expand the stage containing the step
- Auto-scroll to show the step in view

Modified:
- StepTree.vue: added watch() on modelValue with scrollIntoView

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
- `visualization/src/components/StepTree.vue`


## Stats

- **1** files changed
- **46** insertions(+)
- **2** deletions(-)
