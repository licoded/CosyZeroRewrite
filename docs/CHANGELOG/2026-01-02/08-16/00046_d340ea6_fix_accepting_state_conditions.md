# [46] docs: fix accepting state conditions in Tableau DFA doc

**Commit**: `d340ea6` ([`d340ea6ef972c5100d5dc916dc81783051506c1c`](https://github.com/anthropics/cosy-zero/commit/d340ea6ef972c5100d5dc916dc81783051506c1c))
**Date**: 2026-01-02 09:44:36 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Correct Section 5.1 accepting state conditions:
- Original incorrect: "ψ₂ ∈ Γ 或 ψ₁ ∈ Γ"
- Correct: Just local consistency (which already implies Until rules)
- Add detailed explanation of Until consistency requirements
- Clarify that the Until check in code is redundant after
  local consistency check

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Modified
- `migrationDocs/on_the_fly_synthesis/TABLEAU_DFA.md`


## Stats

- **1** files changed
- **16** insertions(+)
- **5** deletions(-)
