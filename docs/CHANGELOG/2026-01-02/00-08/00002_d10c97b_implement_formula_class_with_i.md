# [2] feat: implement Formula class with immutable design

**Commit**: `d10c97b` ([`d10c97b30aec138975f62d240fff8effae64ef51`](https://github.com/anthropics/cosy-zero/commit/d10c97b30aec138975f62d240fff8effae64ef51))
**Date**: 2026-01-02 01:04:10 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- OpType enum for LTLf operators
- Immutable formula representation
- Cached hash for efficient comparisons
- Virtual methods for transformations (nnf, simplify, xnf, rmnext)

## Changes

### Added
- `include/formula/formula.hpp`
- `include/formula/formula_pool.hpp`


## Stats

- **2** files changed
- **551** insertions(+)
- **0** deletions(-)
