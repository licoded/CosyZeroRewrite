# [16] fix: parser single-char operator precedence bug (#002)

**Commit**: `dba0871` ([`dba0871f3bc5fceeaa7e1edcc63898369d12ed11`](https://github.com/anthropics/cosy-zero/commit/dba0871f3bc5fceeaa7e1edcc63898369d12ed11))
**Date**: 2026-01-02 10:22:17 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Fixed bug where variables starting with 'r', 'f', or 'g' were
incorrectly parsed as Release/Finally/Globally operators.

Changes:
- Added peek-ahead logic in tokenizer to check if r/f/g is followed
  by another letter before treating it as an operator
- Added 5 regression tests covering r/f/g-prefixed variables
- Updated docs/BUGS/open.md (0 open bugs now)
- Updated docs/BUGS/fixed.md with bug #002 details
- Updated docs/TODO/parser.md to remove fixed item

Before: "req" → parsed as Release + "eq" (error)
After:  "req" → parsed as identifier "req" (correct)

Test results: 171 assertions in 36 test cases (was 124 in 31)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `docs/BUGS/fixed.md`
- `docs/BUGS/open.md`
- `docs/TODO/parser.md`
- `src/formula/formula_parser.cpp`
- `tests/parser_checker_tests.cpp`


## Stats

- **5** files changed
- **224** insertions(+)
- **91** deletions(-)
