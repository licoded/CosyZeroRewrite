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

## Changes

### Modified
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **232** insertions(+)
- **67** deletions(-)
