# [6] feat: implement formula simplification

**Commit**: `cff6ef4` ([`cff6ef42e53dc64ec2a87e2088a5f8a9745aa84f`](https://github.com/anthropics/cosy-zero/commit/cff6ef42e53dc64ec2a87e2088a5f8a9745aa84f))
**Date**: 2026-01-02 01:04:54 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- O(n) HashSet-based deduplication (improved from O(n log n))
- Algebraic rules: a&True->a, a|False->a, etc.
- Until/Release specific optimizations
- Conflict detection for literals (a & !a -> False)

## Changes

### Added
- `src/formula/simplify.cpp`


## Stats

- **1** files changed
- **410** insertions(+)
- **0** deletions(-)
