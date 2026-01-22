# [78] fix: replace std::async with std::thread to avoid future destructor blocking

**Commit**: `1ec1445` ([`1ec1445666011fb67db1df2c3dc8eed9a107356c`](https://github.com/licoded/CosyZeroRewrite/commit/1ec1445666011fb67db1df2c3dc8eed9a107356c))
**Date**: 2026-01-02 13:38:03 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Use std::thread with detach() instead of std::async for timeout handling
- std::future destructor blocks until task completes, causing hangs
- Reduced polling interval from 100ms to 10ms for better timing accuracy
- Move timing start before thread creation for accurate measurement
- Verified multi-threaded results are consistent across runs

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
修复 std::async 阻塞问题：使用 std::thread + detach() 替代 std::async，避免 future 析构函数阻塞导致挂起。

### 🔍 Technical Details

**问题原因**：
- `std::future` 析构函数会阻塞直到任务完成
- 在超时场景下导致程序挂起

**修复方案**：
- 使用 `std::thread` + `detach()` 替代 `std::async`
- 轮询间隔从 100ms 减少到 10ms 提高计时精度
- 计时起点移到线程创建前确保测量准确

## Changes

### Modified
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **232** insertions(+)
- **67** deletions(-)
