# [6] docs: add known bugs section to CLAUDE.md

**Commit**: `32c749e` ([`32c749e2ac7adc56f2b57757ad164431ae971170`](https://github.com/anthropics/cosy-zero/commit/32c749e2ac7adc56f2b57757ad164431ae971170))
**Date**: 2026-01-02 10:12:55 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Record parser bug with single-char operator precedence:
- r/f/g as first char of variable name triggers operator token
- Temporary workaround: use alternative variable names
- Fix proposal included with code example

Work habit reminder: Always document bugs and limitations!

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `CLAUDE.md`


## Stats

- **1** files changed
- **42** insertions(+)
- **0** deletions(-)
