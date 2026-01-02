# [72] fix: add remote-ahead check before auto-push

**Commit**: `6e306cf` ([`6e306cfad22c6ed3a30c7d3d9560bb431e4f2578`](https://github.com/licoded/CosyZeroRewrite/commit/6e306cfad22c6ed3a30c7d3d9560bb431e4f2578))
**Date**: 2026-01-02 11:37:59 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Skip auto-push if remote dev is ahead of local (e.g., after
rebase or rollback). This prevents overwriting remote history
with local commits that are behind.

## AI Analysis

### 📝 Change Summary
安全检查：在 auto-push 前检查远程是否领先本地，防止 rebase/rollback 后覆盖远程历史。

### 🔍 Technical Details

**场景**：
- 执行 rebase 或回退后，本地落后于远程
- 如果继续 auto-push 会强制覆盖远程历史

**修复**：使用 `git merge-base --is-ancestor` 检查本地是否落后于远程，如果落后则跳过推送。

## Changes

### Modified
- `scripts/post-commit-hook.sh`


## Stats

- **1** files changed
- **16** insertions(+)
- **6** deletions(-)
