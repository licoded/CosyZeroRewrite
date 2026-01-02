# [7] feat: implement rmnext formula progression

**Commit**: `c900f2e` ([`c900f2e52173f10ce8e69f247f9ad44e43e1c577`](https://github.com/anthropics/cosy-zero/commit/c900f2e52173f10ce8e69f247f9ad44e43e1c577))
**Date**: 2026-01-02 01:04:57 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Progress formulas to next state given edge assignment
- Distribute over AND/OR with short-circuit
- Handle End marker for finite traces
- O(n) time complexity

## Changes

### Added
- `src/formula/rmnext.cpp`


## Stats

- **1** files changed
- **239** insertions(+)
- **0** deletions(-)
