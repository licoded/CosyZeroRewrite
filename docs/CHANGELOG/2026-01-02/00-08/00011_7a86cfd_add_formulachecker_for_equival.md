# [11] Add FormulaChecker for equivalence and property checking

**Commit**: `7a86cfd` ([`7a86cfd068c0175a07b087ad2ab686d90e39f638`](https://github.com/anthropics/cosy-zero/commit/7a86cfd068c0175a07b087ad2ab686d90e39f638))
**Date**: 2026-01-02 01:31:36 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implementation includes:
- are_equivalent(): Exact equivalence via truth table (≤4 vars)
- likely_equivalent(): Statistical equivalence via sampling (>4 vars)
- is_nnf(): Negation Normal Form property checker
- is_xnf(): neXt Normal Form property checker (allows U/R inside Next)
- Formula analysis: size, depth, variables, literals, primitives

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `include/formula/formula_checker.hpp`
- `src/formula/formula_checker.cpp`


## Stats

- **2** files changed
- **560** insertions(+)
- **0** deletions(-)
