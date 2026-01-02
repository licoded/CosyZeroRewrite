# [71] fix: exclude CHANGELOG commits from push buffer count

**Commit**: `4686925` ([`46869258b78731dc655da4a0574246b1577290f5`](https://github.com/licoded/CosyZeroRewrite/commit/46869258b78731dc655da4a0574246b1577290f5))
**Date**: 2026-01-02 11:31:34 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Now the hook correctly counts only non-CHANGELOG commits when
determining the 3-commit buffer for amend/rebase workflow.

This keeps the latest 3 real commits (and their CHANGELOGs) local,
while pushing older history to remote.

## AI Analysis

### 📝 Change Summary
修复 post-commit hook：排除 CHANGELOG 提交计数，确保 amend/rebase 工作流中只计算实际提交的 3 提交缓冲区。

### 🔍 Technical Details

**问题**：之前的计数逻辑将 CHANGELOG 自动提交也计入缓冲，导致实际的代码提交少于 3 个就被推送。

**修复**：只计算非 CHANGELOG 提交，保持最新 3 个真实提交（及其 CHANGELOG）在本地。

## Changes

### Modified
- `scripts/post-commit-hook.sh`


## Stats

- **1** files changed
- **18** insertions(+)
- **4** deletions(-)
