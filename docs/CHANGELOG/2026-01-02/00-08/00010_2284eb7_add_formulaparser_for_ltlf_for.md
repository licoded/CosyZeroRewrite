# [10] Add FormulaParser for LTLf formula string parsing

**Commit**: `2284eb7` ([`2284eb77ed9f9c371fa89ba5aaeb03f82bd875db`](https://github.com/anthropics/cosy-zero/commit/2284eb77ed9f9c371fa89ba5aaeb03f82bd875db))
**Date**: 2026-01-02 01:31:32 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implement recursive descent parser supporting:
- Boolean operators: ! (not), & (and), | (or)
- Temporal operators: X (next), U (until), R (release)
- Parentheses for grouping
- Constants: true, false
- Auto-declaration of single-char variables
- Multi-char variables via set_variables()

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `include/formula/formula_parser.hpp`
- `src/formula/formula_parser.cpp`


## Stats

- **2** files changed
- **488** insertions(+)
- **0** deletions(-)
