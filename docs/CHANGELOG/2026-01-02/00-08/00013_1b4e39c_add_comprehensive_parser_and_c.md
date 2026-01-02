# [13] Add comprehensive parser and checker tests

**Commit**: `1b4e39c` ([`1b4e39cb2e634c77e38dd52ecb3c14aa741d4dc9`](https://github.com/anthropics/cosy-zero/commit/1b4e39cb2e634c77e38dd52ecb3c14aa741d4dc9))
**Date**: 2026-01-02 01:31:44 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Parser tests:
- Simple literals and constants
- Negation, And, Or, Next, Until, Release
- Complex formulas with parentheses
- Multi-char and auto-declared variables

Property tests:
- NNF transformation preserves semantics and produces NNF
- XNF transformation preserves semantics and produces XNF
- Simplify preserves semantics, doesn't increase size, is idempotent
- Equivalence checking correctness (reflexivity, symmetry)

Total: 62 test cases, 190 assertions

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `tests/parser_checker_tests.cpp`


## Stats

- **1** files changed
- **444** insertions(+)
- **0** deletions(-)
