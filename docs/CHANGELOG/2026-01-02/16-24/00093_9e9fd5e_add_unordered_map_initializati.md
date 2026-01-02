# [93] docs: add unordered_map initialization explanation to components.md

**Commit**: `9e9fd5e` ([`9e9fd5eaf0f481e5bde57a1d06eedb11f46293b7`](https://github.com/licoded/CosyZeroRewrite/commit/9e9fd5eaf0f481e5bde57a1d06eedb11f46293b7))
**Date**: 2026-01-02 17:41:28 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added section 6.1 explaining:
- unordered_map constructor parameters (bucket_count, hash, equal)
- Why bucket_count=16 is used (performance optimization)
- Meaning of {} syntax for value initialization
- Complete example with CacheKey, CacheKeyHash, CacheKeyEqual

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
<!-- TODO: Add a brief summary of the change in Chinese or English -->

### 🔍 Technical Details
<!-- Optional: Add technical details, root cause, or implementation notes -->

### 📊 Impact Analysis
<!-- Optional: Add impact scope, affected components, or performance notes -->

## Changes

### Modified
- `docs/ARCHITECTURE/components.md`


## Stats

- **1** files changed
- **122** insertions(+)
- **0** deletions(-)
