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

## Changes

### Modified
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **18** insertions(+)
- **16** deletions(-)
