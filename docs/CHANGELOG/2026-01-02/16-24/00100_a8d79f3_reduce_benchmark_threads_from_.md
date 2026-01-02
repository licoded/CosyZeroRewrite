# [100] config: reduce benchmark threads from 8 to 6

**Commit**: `a8d79f3` ([`a8d79f3f7cb519922fdbe1e1bdf714a69726ea89`](https://github.com/licoded/CosyZeroRewrite/commit/a8d79f3f7cb519922fdbe1e1bdf714a69726ea89))
**Date**: 2026-01-02 20:10:51 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

6 threads is a better balance for system stability.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
将 benchmark runner 的并发线程数从 8 降低到 6，以提高系统稳定性。

### 🔍 Technical Details
- 修改 `tools/benchmark_runner.cpp` 中的 `NUM_THREADS` 常量
- 6 线程是更好的系统资源平衡

### 📊 Impact Analysis
- 降低系统负载，避免资源争抢
- 提高长时间运行的稳定性

## Changes

### Modified
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **1** insertions(+)
- **1** deletions(-)
