# [193] feat: add final state capture in trace export

**Commit**: `800c4da` ([`800c4da516d24e3032b7fc9c5e861a0fe132d99f`](https://github.com/licoded/CosyZeroRewrite/commit/800c4da516d24e3032b7fc9c5e861a0fe132d99f))
**Date**: 2026-01-04 15:15:26 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add finalize(bool, const OnTheFlyGameSolver&) overload to capture final state
- Create "final" stage with complete game graph and final classifications
- Update on_the_fly_solver.cpp to pass solver when finalizing trace
- Users can now see the final state graph after all synthesis steps complete

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
- `include/synthesis/trace_exporter.hpp`
- `src/synthesis/on_the_fly_solver.cpp`
- `src/synthesis/trace_exporter.cpp`


## Stats

- **3** files changed
- **56** insertions(+)
- **1** deletions(-)
