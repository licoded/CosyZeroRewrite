# [126] refactor: extract SCC topological sort into separate function

**Commit**: `71543d8` ([`71543d8835dec0a30498252dbe73c54a38fce1e3`](https://github.com/licoded/CosyZeroRewrite/commit/71543d8835dec0a30498252dbe73c54a38fce1e3))
**Date**: 2026-01-03 08:58:31 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Extract SCC graph building and topological sorting logic into
get_scc_processing_order() for better code organization.

- classify_states() reduced from ~50 lines to ~8 lines
- Build SCC graph and compute in-degree in single pass
- Return reverse topological order directly

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
- `include/synthesis/game_solver.hpp`
- `src/synthesis/game_solver.cpp`


## Stats

- **2** files changed
- **108** insertions(+)
- **41** deletions(-)
