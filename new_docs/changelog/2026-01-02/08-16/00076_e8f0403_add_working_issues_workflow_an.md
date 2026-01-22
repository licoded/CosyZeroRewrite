# [76] docs: add working_issues workflow and bug report documentation

**Commit**: `e8f0403` ([`e8f04039ab36de39bff629e42af1a187870acb5f`](https://github.com/licoded/CosyZeroRewrite/commit/e8f04039ab36de39bff629e42af1a187870acb5f))
**Date**: 2026-01-02 13:06:37 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Adds collaborative workflow documentation for complex bug fixing:
- working_issues/ directory structure for detailed bug tracking
- Bug report for LTLf synthesis accuracy issues (now resolved)
- CLAUDE.md updates for git commit workflow during bug fixes

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
协作工作流文档：添加 working_issues 目录结构和 bug 报告模板，用于复杂 bug 的追踪和协作修复。

### 🔍 Technical Details

**working_issues/ 目录结构**：
```
docs/working_issues/
└── YYYY-MM-DD_AM/PM_ProblemSummary/
    └── BUG_REPORT.md
```

**BUG_REPORT.md 包含**：
- 问题描述
- 根因分析
- 已修复的 bug 列表
- 进行中的问题
- 测试结果

**工作流改进**：
- 更新 CLAUDE.md 添加 git 提交规范
- 支持分阶段提交便于回滚和代码审查

## Changes

### Added
- `docs/working_issues/2026-01-02_PM_BenchmarkAccuracy/BUG_REPORT.md`


### Modified
- `CLAUDE.md`


## Stats

- **2** files changed
- **242** insertions(+)
- **0** deletions(-)
