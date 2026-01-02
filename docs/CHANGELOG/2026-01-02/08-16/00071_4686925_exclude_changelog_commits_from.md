# [71] fix: exclude CHANGELOG commits from push buffer count

**Commit**: `4686925` ([`46869258b78731dc655da4a0574246b1577290f5`](https://github.com/licoded/CosyZeroRewrite/commit/46869258b78731dc655da4a0574246b1577290f5))
**Date**: 2026-01-02 11:31:34 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Now the hook correctly counts only non-CHANGELOG commits when
determining the 3-commit buffer for amend/rebase workflow.

This keeps the latest 3 real commits (and their CHANGELOGs) local,
while pushing older history to remote.

## Changes

### Modified
- `scripts/post-commit-hook.sh`


## Stats

- **1** files changed
- **18** insertions(+)
- **4** deletions(-)
