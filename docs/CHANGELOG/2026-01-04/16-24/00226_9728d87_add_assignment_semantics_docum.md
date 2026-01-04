# [226] docs: add Assignment Semantics documentation and time record requirement

**Commit**: `9728d87` ([`9728d87cfef317801f11545e5ffd462868f3b0c9`](https://github.com/licoded/CosyZeroRewrite/commit/9728d87cfef317801f11545e5ffd462868f3b0c9))
**Date**: 2026-01-04 23:17:19 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added documentation for Assignment Semantics design decision:
- sigma (σ) only contains variables set to TRUE
- Unselected variables are implicitly FALSE
- sys_move={p1} ≡ sys_move={p1, !p5}

Updated:
- src/automata/tableau.cpp: Added detailed comment (2026-01-04)
- docs/ARCHITECTURE/on_the_fly_algorithm.md: Added section 2.3 (2026-01-04)
- CLAUDE.md: Added documentation modification规范 (2026-01-04)

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
- `CLAUDE.md`
- `docs/ARCHITECTURE/on_the_fly_algorithm.md`
- `src/automata/tableau.cpp`


## Stats

- **3** files changed
- **98** insertions(+)
- **1** deletions(-)
