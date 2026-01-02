# [12] feat: add AI-powered CHANGELOG analysis

**Commit**: `9977380` ([`9977380433aa67182a5439c79b5fb525b267f0bf`](https://github.com/anthropics/cosy-zero/commit/9977380433aa67182a5439c79b5fb525b267f0bf))
**Date**: 2026-01-02 10:49:02 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add ai-changelog.sh script to extract diff for Claude analysis
- Add Makefile with convenient targets (changelog, ai-changelog, etc.)
- Update CHANGELOG README with AI analysis documentation
- Add AI Analysis section to example CHANGELOGs (00029, 00030)
- Include mermaid flowchart for Until semantics in 00030

Usage:
  make ai-changelog    # Analyze latest commit
  make changelog       # Generate basic changelog

## Changes

### Added
- `docs/CHANGELOG/00030_97e2f8f_correct_accepting_state_condit.md`
- `Makefile`
- `scripts/ai-changelog.sh`


### Modified
- `docs/CHANGELOG/00029_059f9a0_add_changelog_and_refine_skip_.md`
- `docs/CHANGELOG/README.md`


## Stats

- **5** files changed
- **361** insertions(+)
- **2** deletions(-)
