# [241] feat: add --sleep parameter to benchmark_test for testing progress bar

**Commit**: `1e9352d` ([`1e9352dde5c1d21a13513f6994a4551b3173ca89`](https://github.com/licoded/CosyZeroRewrite/commit/1e9352dde5c1d21a13513f6994a4551b3173ca89))
**Date**: 2026-01-05 00:39:00 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add --sleep option to artificially slow down each task, allowing
observation of the progress bar behavior during parallel execution.

Changes:
- Add --sleep parameter (0-60 seconds) to CLI11 options
- Pass sleep_per_task to BenchmarkRunner and process_formula
- Use std::this_thread::sleep_for to delay after each parse
- Include <thread> header for sleep functionality

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
添加 `--sleep` 参数用于测试进度条显示效果。允许用户在执行每个任务后人为延迟指定秒数，便于观察并发执行时的进度条行为。

### 🔍 Technical Details
- 使用 `std::this_thread::sleep_for()` 实现任务延迟
- 参数范围：0-60 秒（通过 CLI::Range 验证）
- 延迟在公式解析完成后执行，不影响实际测量时间
- 仅为调试/演示目的，不影响 benchmark 功能

### 📊 Impact Analysis
- 影响组件：`tests/bench/benchmark_test`
- 功能增强：便于观察并发执行和进度条更新
- 无性能影响（默认值为 0，不启用延迟）

## Changes

### Modified
- `tests/bench/benchmark.cpp`


## Stats

- **1** files changed
- **82** insertions(+)
- **48** deletions(-)
