# [153] refactor: disable debug output by default

**Commit**: `307f41d` ([`307f41dc2f8cd1fa861ff20309947aaf07299138`](https://github.com/licoded/CosyZeroRewrite/commit/307f41dc2f8cd1fa861ff20309947aaf07299138))
**Date**: 2026-01-03 23:34:21 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Changes:
- tableau.cpp: debug_log() now controlled by COSY_DEBUG_TABLEAU=1 env var
- on_the_fly_solver.cpp: classify_scc debug controlled by COSY_DEBUG_CLASSIFY=1
- Default: OFF (no debug spam in test output)
- Enable: export COSY_DEBUG_TABLEAU=1 for tableau debug
         export COSY_DEBUG_CLASSIFY=1 for SCC classification debug

Test output is now clean - failures clearly visible:
- make test shows which tests failed
- Individual tests show PASS/FAILED/SKIP clearly

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
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **2** files changed
- **23** insertions(+)
- **6** deletions(-)
