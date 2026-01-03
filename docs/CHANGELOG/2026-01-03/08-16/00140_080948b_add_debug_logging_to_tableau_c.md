# [140] feat: add debug logging to tableau (console + file)

**Commit**: `080948b` ([`080948b92633bda44c31f94ca99b823b53f668a9`](https://github.com/licoded/CosyZeroRewrite/commit/080948b92633bda44c31f94ca99b823b53f668a9))
**Date**: 2026-01-03 11:02:23 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Added debug_log() function that writes to both stderr and /tmp/tableau_debug.log
- Logs formula progression details in next_phi()
- Logs state creation in get_or_create()

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
- `src/automata/tableau.cpp`


## Stats

- **2** files changed
- **72** insertions(+)
- **1** deletions(-)
