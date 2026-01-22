# [188] fix: correct trace JSON format and directory structure

**Commit**: `db70fc3` ([`db70fc31100d7e7a87ee232bd4c3e7d608cacee7`](https://github.com/licoded/CosyZeroRewrite/commit/db70fc31100d7e7a87ee232bd4c3e7d608cacee7))
**Date**: 2026-01-04 14:32:10 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Only include state_data field when non-empty (fixes JSON parsing)
- Change directory structure from results/trace_YYYYMMDD_HHMMSS/ to results/trace/YYYY-MM-DD/HH-period/ to match game_graph format
- Make state_data optional in TypeScript types

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
- `visualization/src/types/trace.ts`


## Stats

- **2** files changed
- **38** insertions(+)
- **36** deletions(-)
