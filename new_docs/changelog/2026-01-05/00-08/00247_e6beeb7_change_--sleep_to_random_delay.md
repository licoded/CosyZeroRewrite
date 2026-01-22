# [247] feat: change --sleep to random delay with average value

**Commit**: `e6beeb7` ([`e6beeb7b896969565008941ec1dbc8c0168dec0b`](https://github.com/licoded/CosyZeroRewrite/commit/e6beeb7b896969565008941ec1dbc8c0168dec0b))
**Date**: 2026-01-05 00:46:38 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Change --sleep parameter from fixed delay to random delay:
- Random sleep time: 0 to 2*sleep (uniform distribution)
- Average sleep time equals the --sleep value
- Each thread has its own RNG (thread_local mt19937)

This makes concurrent task completion more realistic for
observing the progress bar behavior.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
将 `--sleep` 参数从固定延迟改为随机延迟，使并发任务完成时间更加真实，便于观察进度条的实时更新行为。

### 🔍 Technical Details
- 随机范围：0 到 2×sleep 秒（均匀分布）
- 平均延迟：等于 `--sleep` 参数值
- 使用 `thread_local std::mt19937` 保证线程安全
- 每个线程有独立的 RNG 实例，避免竞争

### 📊 Impact Analysis
- 影响组件：`benchmark_test --sleep` 参数
- 行为改变：从固定延迟变为随机延迟
- 用途：模拟真实场景中任务完成时间的差异

## Changes

### Modified
- `tests/bench/benchmark.cpp`


## Stats

- **1** files changed
- **9** insertions(+)
- **4** deletions(-)
