# [15] fix: correct temporal operator evaluation in equivalence checker

**Commit**: `38c53bc` ([`38c53bcc90d6c446b8fd6f324767cd524c38c333`](https://github.com/anthropics/cosy-zero/commit/38c53bcc90d6c446b8fd6f324767cd524c38c333))
**Date**: 2026-01-02 01:39:41 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Fixed truth table evaluation for Until and Release operators to match
their LTLf semantics:

- Until (f U g): In single-point evaluation, this reduces to just g
  (whether g is true NOW), since there's no next state for X(f U g)
- Release (f R g): If g is true, the formula is always satisfied since
  g must hold from the current point onward

Also fixed fuzz_parser.cpp to use correct method names (to_string).

Bug found by libFuzzer testing.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `src/formula/formula_checker.cpp`
- `tests/fuzz/fuzz_parser.cpp`


## Stats

- **2** files changed
- **20** insertions(+)
- **18** deletions(-)
