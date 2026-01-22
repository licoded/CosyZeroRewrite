# [60] fix: prevent CHANGELOG infinite loop

**Commit**: `184c70a` ([`184c70a0296982fe8e30b54174e96e424d21fb36`](https://github.com/licoded/CosyZeroRewrite/commit/184c70a0296982fe8e30b54174e96e424d21fb36))
**Date**: 2026-01-02 10:45:03 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add detection for CHANGELOG-only commits to skip generating more CHANGELOGs
- Strip ANSI escape codes from output for proper filepath extraction
- Auto-commit new CHANGELOG files in follow-up commits

## AI Analysis

### 📝 Change Summary
修复 prevent CHANGELOG infinite loop。

## Changes

### Modified
- `scripts/post-commit-hook.sh`


## Stats

- **1** files changed
- **49** insertions(+)
- **4** deletions(-)
