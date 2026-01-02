# [72] fix: add remote-ahead check before auto-push

**Commit**: `6e306cf` ([`6e306cfad22c6ed3a30c7d3d9560bb431e4f2578`](https://github.com/licoded/CosyZeroRewrite/commit/6e306cfad22c6ed3a30c7d3d9560bb431e4f2578))
**Date**: 2026-01-02 11:37:59 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Skip auto-push if remote dev is ahead of local (e.g., after
rebase or rollback). This prevents overwriting remote history
with local commits that are behind.

## Changes

### Modified
- `scripts/post-commit-hook.sh`


## Stats

- **1** files changed
- **16** insertions(+)
- **6** deletions(-)
