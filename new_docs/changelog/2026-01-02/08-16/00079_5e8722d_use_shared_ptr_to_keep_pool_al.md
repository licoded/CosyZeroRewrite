# [79] fix: use shared_ptr to keep pool alive for detached threads

**Commit**: `5e8722d` ([`5e8722df1773806062da13efc9498e70994bef30`](https://github.com/licoded/CosyZeroRewrite/commit/5e8722df1773806062da13efc9498e70994bef30))
**Date**: 2026-01-02 13:41:42 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Changed pool allocation to heap using shared_ptr
- Also made syn_result and done use shared_ptr
- Prevents crash when detached thread accesses destroyed locals
- Now safely supports timeout with background cleanup

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
修复 detached 线程崩溃：使用 shared_ptr 管理 pool、syn_result 和 done 生命周期，防止 detached 线程访问已销毁的局部变量。

### 🔍 Technical Details

**问题原因**：
- detached 线程可能访问已销毁的局部变量
- 当线程超时时，主线程返回，局部变量析构
- detached 后台线程继续访问导致崩溃

**修复方案**：
- `pool` 改用 `shared_ptr` 在堆上分配
- `syn_result` 和 `done` 也使用 `shared_ptr`
- 引用计数确保变量在线程完成前不会被销毁

## Changes

### Modified
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **18** insertions(+)
- **16** deletions(-)
