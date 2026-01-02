# [18] Implement Bounded Model Checking (BMC) for LTLf in Z3 integration

**Commit**: `68f1692` ([`68f1692f33f25ed38ce046184de784dc2836e233`](https://github.com/anthropics/cosy-zero/commit/68f1692f33f25ed38ce046184de784dc2836e233))
**Date**: 2026-01-02 01:53:19 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Uses time unrolling to handle temporal operators (U/R/X):
- Each proposition p becomes time-indexed: p_0, p_1, ..., p_bound
- X f at time t: f at time t+1
- f U g at time t: disjunction over all satisfaction points
- f R g at time t: g holds continuously until f becomes true

Bound selection strategies:
- max_bound = -1: incremental checking (start small, increase)
- max_bound = 0: auto-detect from formula structure
- max_bound > 0: use specified bound

Heuristics:
- X depth: count nested X operators (minimum bound needed)
- U/R depth: structural depth + X depth + 2

Fixed Release semantics: g holds continuously until f becomes true
(not just g OR f_held at each time point).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `include/formula/formula_z3.hpp`
- `src/formula/formula_checker.cpp`
- `src/formula/formula_z3.cpp`


## Stats

- **3** files changed
- **288** insertions(+)
- **145** deletions(-)
