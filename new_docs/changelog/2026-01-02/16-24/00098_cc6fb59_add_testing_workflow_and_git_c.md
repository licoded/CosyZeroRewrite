# [98] docs: add testing workflow and git commit guidelines to CLAUDE.md

**Commit**: `cc6fb59` ([`cc6fb5995c409f297ae2ab507ff35ad2c9d0d512`](https://github.com/licoded/CosyZeroRewrite/commit/cc6fb5995c409f297ae2ab507ff35ad2c9d0d512))
**Date**: 2026-01-02 20:08:02 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add explicit testing sequence: unit tests → small sample → full benchmark
- Add git commit sequence: commit first, then test
- Add examples and reminders

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
在 CLAUDE.md 中添加了明确的测试流程规范和 Git 提交规范，确保开发过程中遵循正确的测试顺序和提交习惯。

### 🔍 Technical Details
- 添加了三阶段测试流程：单元测试 → 小范围抽查 → 全量 benchmark
- 明确了修改代码后的提交顺序：先提交代码，再运行测试
- 提供了具体命令示例和注意事项

### 📊 Impact Analysis
- 防止在有明显问题的情况下运行全量 benchmark，节省时间
- 确保每次代码修改都有可回退的 git 快照
- 提高开发效率和代码质量

## Changes

### Modified
- `CLAUDE.md`


## Stats

- **1** files changed
- **74** insertions(+)
- **0** deletions(-)
