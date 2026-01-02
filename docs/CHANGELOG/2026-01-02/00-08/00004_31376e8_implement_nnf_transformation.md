# [4] feat: implement NNF transformation

**Commit**: `31376e8` ([`31376e81dfd38188b700dd21b493cd467197ef63`](https://github.com/anthropics/cosy-zero/commit/31376e81dfd38188b700dd21b493cd467197ef63))
**Date**: 2026-01-02 01:04:48 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Push negations inward using De Morgan's laws
- Double negation elimination
- Temporal operator duality (U/R, F/G)
- LTLf Next negation with End marker: \!X(a) -> X(\!a) | End
- O(n) time complexity

## Changes

### Added
- `src/formula/nnf.cpp`


## Stats

- **1** files changed
- **129** insertions(+)
- **0** deletions(-)
