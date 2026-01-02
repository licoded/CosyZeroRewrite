# [5] feat: implement XNF transformation

**Commit**: `46cb0d4` ([`46cb0d4b791375d2d2ec423724e7502dc83a362e`](https://github.com/anthropics/cosy-zero/commit/46cb0d4b791375d2d2ec423724e7502dc83a362e))
**Date**: 2026-01-02 01:04:51 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Expand Until/Release to top level
- Eventually true (♢true) = !End
- Always false (□false) = End
- No recursion on U/R inside Next (handled by rmnext)
- O(n) single-pass expansion

## Changes

### Added
- `src/formula/xnf.cpp`


## Stats

- **1** files changed
- **119** insertions(+)
- **0** deletions(-)
