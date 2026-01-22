# [212] feat: add label field to new_edges in trace export

**Commit**: `ef85943` ([`ef859436ec887c50437d8deaba22021755119a2a`](https://github.com/licoded/CosyZeroRewrite/commit/ef859436ec887c50437d8deaba22021755119a2a))
**Date**: 2026-01-04 21:05:48 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add edge labels (sys={...}, env={...}) to highlights.new_edges
in trace JSON. For env moves, convert local indices to global
indices when formatting variable names.

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
- `src/synthesis/on_the_fly_solver.cpp`
- `src/synthesis/trace_exporter.cpp`


## Stats

- **2** files changed
- **63** insertions(+)
- **2** deletions(-)
