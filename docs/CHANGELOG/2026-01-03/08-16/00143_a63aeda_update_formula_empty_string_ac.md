# [143] docs: update formula_empty_string_accepting logic and comments

**Commit**: `a63aeda` ([`a63aedad15c76a2ad55c1d2a6d8ec9cc26cd5f4a`](https://github.com/licoded/CosyZeroRewrite/commit/a63aedad15c76a2ad55c1d2a6d8ec9cc26cd5f4a))
**Date**: 2026-01-03 11:40:43 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Update ESA logic based on new understanding:
- Release: returns true (weak semantics, holds by default)
- Not: returns false (simplified, TODO for proper handling)
- Literal/False: return false (needs state to evaluate)
- Update function and inline comments to reflect the new semantics

Test results:
- G p1 (Release): now PASS
- p1 & !p1 (contradiction): now PASS
- X p1: still FAIL (needs further analysis)

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


## Stats

- **1** files changed
- **20** insertions(+)
- **30** deletions(-)
