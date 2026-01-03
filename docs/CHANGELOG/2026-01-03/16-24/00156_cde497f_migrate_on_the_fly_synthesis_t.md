# [156] refactor: migrate on_the_fly_synthesis_tests to Catch2 framework

**Commit**: `cde497f` ([`cde497f1d95a646e3bb9a16ad07f859456fdb059`](https://github.com/licoded/CosyZeroRewrite/commit/cde497f1d95a646e3bb9a16ad07f859456fdb059))
**Date**: 2026-01-03 23:49:54 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Update CMakeLists.txt to link catch2
- Replace hand-written test macros with Catch2 TEST_CASE/REQUIRE
- Add tags for categorization: [basic], [temporal], [unrealizable], [complex]
- Use INFO() to display formula content in test output
- Support COSY_TEST_VERBOSE=1 for detailed output
- Remove ~150 lines of custom test framework code

Benefits:
- Unified testing framework with other tests
- Better error messages and assertion output
- Tag-based test filtering support
- Leverages Catch2's mature testing features

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
- `cmake/Tests.cmake`
- `tests/on_the_fly_synthesis_tests.cpp`


## Stats

- **2** files changed
- **71** insertions(+)
- **268** deletions(-)
