# [205] fix: rewrite classify_scc with correct backward algorithm

**Commit**: `f2c4ed5` ([`f2c4ed53f3234746409aa349ae3653af70531c34`](https://github.com/licoded/CosyZeroRewrite/commit/f2c4ed53f3234746409aa349ae3653af70531c34))
**Date**: 2026-01-04 19:43:27 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Key changes:
- predecessors map: System → System (via complete sys+env move)
- swin_states/tmpSet only contain System states (with asserts)
- Classification: EXISTS sys move where ALL env moves lead to Swin

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
- `docs/ARCHITECTURE/on_the_fly_algorithm.md`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **2** files changed
- **170** insertions(+)
- **103** deletions(-)
