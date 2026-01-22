# [215] fix: correct env state classification when encountering Ewin successors

**Commit**: `c59db9e` ([`c59db9e29f63ca9d056cef1199d312c5303f92b9`](https://github.com/licoded/CosyZeroRewrite/commit/c59db9e29f63ca9d056cef1199d312c5303f92b9))
**Date**: 2026-01-04 21:26:54 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Fixed bug in classify_scc() where all_env_moves_swin was incorrectly
set to true when encountering an Ewin successor. This caused Environment
states with Ewin successors to be misclassified as Swin.

Bug fix: set all_env_moves_swin = false when encountering Ewin or unknown successors.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
修复 `classify_scc` 函数中 Environment 状态分类的错误：当遇到 Ewin 后继时，`all_env_moves_swin` 被错误设置为 true。

### 🔍 Technical Details
- Bug 位置：`on_the_fly_solver.cpp:743`
- 原代码：遇到 Ewin 后继后设置 `all_env_moves_swin = true`（错误）
- 修复：`all_env_moves_swin = false`（正确）
- 影响：Environment 状态如果有 Ewin 后继，现在正确分类为 Ewin

### 📊 Impact Analysis
- 修复前：E0/E1 等状态被错误分类为 Swin（导致 f118 测试结果为 REALIZABLE）
- 修复后：E0/E1 正确分类为 Ewin（f118 测试结果为 UNREALIZABLE，与 Cosy 一致）

## Changes

### Modified
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **1** files changed
- **2** insertions(+)
- **1** deletions(-)
