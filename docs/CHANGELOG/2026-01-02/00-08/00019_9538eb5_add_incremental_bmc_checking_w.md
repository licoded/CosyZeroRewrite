# [19] Add incremental BMC checking with time estimation

**Commit**: `9538eb5` ([`9538eb5eb288e12e0a51b3668b0c3bd597c2a422`](https://github.com/anthropics/cosy-zero/commit/9538eb5eb288e12e0a51b3668b0c3bd597c2a422))
**Date**: 2026-01-02 01:58:14 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Features:
- 8x bound multiplier (configurable via DEFAULT_BOUND_MULTIPLIER)
- Incremental mode: starts with small bounds (2, 4, 8, ...) increases exponentially
- Time estimation: fits power law from actual measurements
- Abort if estimated time exceeds timeout (default 5 minutes)
- Default timeout increased to 300s (5 minutes)

Time estimation model:
- Uses two most recent data points to fit: time = c * bound^k
- Clamps exponent k to [1, 4] for stability
- Falls back to O(bound^2) if insufficient data
- Applies 2x safety factor before aborting

Configuration:
- DEFAULT_BOUND_MULTIPLIER = 8
- DEFAULT_TIMEOUT = 300000ms (5 minutes)
- MIN_INCREMENTAL_START_BOUND = 2
- MAX_INCREMENTAL_STEPS = 10

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `include/formula/formula_z3.hpp`
- `src/formula/formula_z3.cpp`


## Stats

- **2** files changed
- **187** insertions(+)
- **35** deletions(-)
