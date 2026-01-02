# [45] docs: add detailed Tableau DFA construction guide

**Commit**: `9bbc0f9` ([`9bbc0f95aa4c656052756cdc5cf05995a8c2d7a0`](https://github.com/anthropics/cosy-zero/commit/9bbc0f95aa4c656052756cdc5cf05995a8c2d7a0))
**Date**: 2026-01-02 09:18:04 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add comprehensive documentation for tableau-based DFA construction:

- Comparison: Traditional vs Tableau methods
- Local consistency rules (Tableau 1)
- Next-state computation (Tableau 2)
- 4 detailed examples with step-by-step derivation
- Implementation details (hash consing, caching, literal evaluation)
- Accepting state conditions
- Class hierarchy and algorithms
- Complexity analysis
- FAQ section

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `migrationDocs/on_the_fly_synthesis/TABLEAU_DFA.md`


## Stats

- **1** files changed
- **391** insertions(+)
- **0** deletions(-)
