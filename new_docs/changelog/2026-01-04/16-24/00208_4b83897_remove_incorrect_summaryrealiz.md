# [208] fix: remove incorrect summary.realizable from trace JSON

**Commit**: `4b83897` ([`4b838975c915442ed656c4f6c5d24c4f86d8be7b`](https://github.com/licoded/CosyZeroRewrite/commit/4b838975c915442ed656c4f6c5d24c4f86d8be7b))
**Date**: 2026-01-04 20:33:09 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

The summary.realizable was incorrectly set to finalized_ flag
(whether trace was written) instead of actual realizability result.

Frontend now determines realizability from final step's initial
state classification, which is the authoritative source.

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


## Stats

- **1** files changed
- **1** insertions(+)
- **1** deletions(-)
