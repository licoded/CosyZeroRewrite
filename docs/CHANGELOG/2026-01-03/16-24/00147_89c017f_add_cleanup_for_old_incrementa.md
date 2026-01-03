# [147] scripts: add cleanup for old incremental formula log files

**Commit**: `89c017f` ([`89c017fb5c69e44f1548535cd2d630af0995160f`](https://github.com/licoded/CosyZeroRewrite/commit/89c017fb5c69e44f1548535cd2d630af0995160f))
**Date**: 2026-01-03 18:52:36 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Added cleanup_old_formula_logs() function to reorganize_logs.py
- Removes old incremental-named files (formula_1.log, formula_1_2.log, etc.)
- Keeps only formula.log in each period directory
- All old formula logs successfully cleaned up

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
- `scripts/reorganize_logs.py`


## Stats

- **1** files changed
- **57** insertions(+)
- **0** deletions(-)
