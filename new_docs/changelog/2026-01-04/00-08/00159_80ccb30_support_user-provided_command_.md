# [159] test: support user-provided command line args

**Commit**: `80ccb30` ([`80ccb30ea4896ef1e4ea4239ae79f85e98c32227`](https://github.com/licoded/CosyZeroRewrite/commit/80ccb30ea4896ef1e4ea4239ae79f85e98c32227))
**Date**: 2026-01-04 00:02:28 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Merge default args with user-provided args
- Users can now specify reporter (-r console), tags ([basic]), etc.
- If user specifies -r, don't override with compact reporter
- Help options (-h, -l, -t) work correctly

Examples:
  ./on_the_fly_synthesis_tests -l           # list tests
  ./on_the_fly_synthesis_tests [basic]      # run basic tests
  ./on_the_fly_synthesis_tests -r console   # use console reporter

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
- `tests/on_the_fly_synthesis_tests.cpp`


## Stats

- **1** files changed
- **18** insertions(+)
- **11** deletions(-)
