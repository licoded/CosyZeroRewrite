# [42] feat: add benchmark runner for SMv2 test suite

**Commit**: `c5fe850` ([`c5fe8508b5537e2c958d89a5c42f897eeedf371d`](https://github.com/licoded/CosyZeroRewrite/commit/c5fe8508b5537e2c958d89a5c42f897eeedf371d))
**Date**: 2026-01-02 09:14:09 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add benchmark runner for testing synthesis against the SMv2
benchmark set (1000 LTLf synthesis problems).

Features:
- Parse .ltlf formula files
- Parse .part variable partition files
- Compare with reference results from results.csv
- Support for custom test ranges

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
实现 add benchmark runner for SMv2 test suite。

## Changes

### Added
- `tests/benchmark_runner.cpp`


## Stats

- **1** files changed
- **111** insertions(+)
- **0** deletions(-)
