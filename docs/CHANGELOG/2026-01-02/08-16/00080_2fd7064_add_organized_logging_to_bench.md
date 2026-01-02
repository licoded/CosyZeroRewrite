# [80] feat: add organized logging to benchmark runner

**Commit**: `2fd7064` ([`2fd7064449a506cd8eb7d950a9e9c97c26f52b84`](https://github.com/licoded/CosyZeroRewrite/commit/2fd7064449a506cd8eb7d950a9e9c97c26f52b84))
**Date**: 2026-01-02 13:56:05 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Create logs/benchmark/YYYY-MM-DD/HH-MM/ directory structure
- Output is written to both stdout and log file
- Log file named with timestamp: benchmark_YYYYMMDD_HHMMSS.log
- Thread-safe file output with flush after each write

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **66** insertions(+)
- **9** deletions(-)
