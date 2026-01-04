# [224] docs: add detailed bug cases with formula, partition, results

**Commit**: `06d0ff3` ([`06d0ff381e2d0e6217609042bc067bc603bf810a`](https://github.com/licoded/CosyZeroRewrite/commit/06d0ff381e2d0e6217609042bc067bc603bf810a))
**Date**: 2026-01-04 23:00:06 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Bug #001: X(!(p)) with outputs 判断错误
- Formula: X(!(p3))
- Partition: inputs=p3, outputs=p2
- Cosy2: UNREALIZABLE (wrong)
- Cosy: Realizable (expected)
- Type: False Negative

Bug #002: F/G 操作符实现问题
- G(!(p3)), G(p3), X(F(!(p3))) 仍然不一致
- 需要分析 NNF 转换和 progression 实现

Updated docs/BUGS/open.md with complete test case details.

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
- `docs/BUGS/open.md`


## Stats

- **1** files changed
- **75** insertions(+)
- **15** deletions(-)
