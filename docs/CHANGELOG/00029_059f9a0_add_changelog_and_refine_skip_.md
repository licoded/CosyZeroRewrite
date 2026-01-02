# [29] docs: add CHANGELOG and refine skip pattern

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


## AI Analysis

### 📝 Change Summary
修复了 post-commit hook 的误判问题。之前的正则表达式 `^(docs:|chore:).*CHANGELOG` 过于宽泛，导致任何提到 "CHANGELOG" 的文档提交都被跳过（例如 cf9166d "docs: document CHANGELOG infinite loop prevention"）。新正则精确匹配 `docs: (add|update) CHANGELOG for...` 模式。

### 🔍 Technical Details
```bash
# Before (too broad)
^(docs:|chore:).*CHANGELOG

# After (precise)
^docs:\ (add|update)\ CHANGELOG\ for
```
这确保只有自动生成的 CHANGELOG 提交被跳过，而正常的文档提交（即使提到 CHANGELOG）仍会生成对应的 CHANGELOG 记录。

### 📊 Impact Analysis
- **范围**: `.git/hooks/post-commit` (通过脚本安装)
- **影响**: 消除 false positive，所有真实提交都会被记录

## Stats

- **2** files changed
- **23** insertions(+)
- **1** deletions(-)
