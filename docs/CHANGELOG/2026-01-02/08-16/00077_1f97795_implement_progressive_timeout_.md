# [77] feat: implement progressive timeout strategy for benchmark runner

**Commit**: `1f97795` ([`1f977958e72383361a2672b5ea65bf7fa1be491b`](https://github.com/licoded/CosyZeroRewrite/commit/1f977958e72383361a2672b5ea65bf7fa1be491b))
**Date**: 2026-01-02 13:16:13 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Improves benchmark runner with multi-stage timeout approach:
- Stage 1: 1 minute timeout per formula
- Stage 2: 3 minute timeout for previous timeouts
- Stage 3: 5 minute timeout for remaining timeouts
- Progress report after each stage

Technical changes:
- Use std::async with wait_for() for timeout control
- Track which formulas timed out at each stage
- Generate detailed summary with confusion matrix
- Output includes timeout tracking and expanded states count

This allows fast formulas to complete quickly while giving
complex formulas more time as needed.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **168** insertions(+)
- **97** deletions(-)
