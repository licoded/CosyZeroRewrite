# [105] docs: add build command reminder to CLAUDE.md

**Commit**: `174b1ce` ([`174b1ced6b6bd2333513bd22c97dd748559ac566`](https://github.com/licoded/CosyZeroRewrite/commit/174b1ced6b6bd2333513bd22c97dd748559ac566))
**Date**: 2026-01-02 20:32:25 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

记录 CMake 构建命令的正确使用方式：
- 正确: cd build && make 或 cmake --build build
- 错误: 在项目根目录直接运行 make

原因：项目根目录的 Makefile 是辅助工具，
不是 CMake 编译入口。

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


## Stats

- **1** files changed
- **7** insertions(+)
- **0** deletions(-)
