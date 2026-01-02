# [62] docs: add CHANGELOG and refine skip pattern

**Commit**: `059f9a0` ([`059f9a041e8c4a4013df6ad75984a018ba0f67a7`](https://github.com/anthropics/cosy-zero/commit/059f9a041e8c4a4013df6ad75984a018ba0f67a7))
**Date**: 2026-01-02 10:45:39 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Manually add CHANGELOG for cf9166d (was false positive skipped)
- Refine regex to only skip 'docs: add/update CHANGELOG for...' commits

## Changes

### Added
- `docs/CHANGELOG/00028_cf9166d_document_changelog_infinite_lo.md`


### Modified
- `scripts/post-commit-hook.sh`


## Stats

- **2** files changed
- **23** insertions(+)
- **1** deletions(-)
