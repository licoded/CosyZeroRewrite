# [82] docs: add bug report for low benchmark accuracy (64% vs 100%)

**Commit**: `2649666` ([`264966631ef00148c95d3c412c2d7f9e334e70d0`](https://github.com/licoded/CosyZeroRewrite/commit/264966631ef00148c95d3c412c2d7f9e334e70d0))
**Date**: 2026-01-02 14:06:48 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Issue: CosyZeroRewrite achieves only 64% accuracy while Cosy reference gets 100%
Pattern: Systematic reversal of results (R↔U) suggests algorithmic bug
Created detailed bug report with investigation steps

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
Bug 报告创建：记录 SMv1000 benchmark 准确率问题 (CosyZeroRewrite 64% vs Cosy 100%)，错误模式显示结果系统性反转。

### 🔍 Technical Details

**问题描述**：
- CosyZeroRewrite: 64% 准确率
- Cosy 参考: 100% 准确率
- 错误模式: Realizable ↔ Unrealizable 系统性反转

**影响**：
- 表明存在算法级别的问题
- 需要深入调查 SCC 分类逻辑、I/O 分离处理、Release 公式语义

**后续工作**：
- 此问题在 00083 和 00084 中得到修复

## Changes

### Added
- `docs/working_issues/2026-01-02_PM_AccuracyIssue/BUG_REPORT.md`


## Stats

- **1** files changed
- **78** insertions(+)
- **0** deletions(-)
