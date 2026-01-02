# [23] test: add logger integration to test suites

**Commit**: `7e4c393` ([`7e4c3938e4a9a067f097669d4e090a100270f50e`](https://github.com/anthropics/cosy-zero/commit/7e4c3938e4a9a067f097669d4e090a100270f50e))
**Date**: 2026-01-02 02:46:36 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Use CATCH_CONFIG_RUNNER for custom main
- Add LOG_INFO for test start/completion
- Add LOG_FLUSH before exit

## Changes

### Modified
- `tests/formula_tests.cpp`
- `tests/parser_checker_tests.cpp`


## Stats

- **2** files changed
- **40** insertions(+)
- **1** deletions(-)
