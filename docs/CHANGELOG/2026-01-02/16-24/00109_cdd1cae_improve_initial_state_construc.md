# [109] fix: improve initial_state construction and add XNF documentation

**Commit**: `cdd1cae` ([`cdd1cae4be6a8df16792deb5c4ec668782339b37`](https://github.com/licoded/CosyZeroRewrite/commit/cdd1cae4be6a8df16792deb5c4ec668782339b37))
**Date**: 2026-01-02 21:29:48 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Fix: Don't expand child of Not operators in initial_state (fixes !p1 case)
- Fix: Empty DFA states now correctly marked as accepting
- Fix: Terminal System states now check is_accepting for classification
- Add: XNF paper and transformation documentation
- Add: DOT graph visualization guide for debugging

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary

修复 initial_state 构造逻辑中的 bug，解决了 `!p1` (系统变量) 返回错误结果的问题。添加了 XNF 论文和转换规则文档。

### 🔍 Technical Details

**Bug 修复**:
1. `initial_state` 中不再展开 Not 操作符的子公式，避免 `!p1` 被错误展开为 `{p1, !p1}`
2. 空 DFA 状态现在正确标记为 accepting（LTLf 语义）
3. 终端 System 状态现在根据 `is_accepting` 正确分类为 Swin 或 Ewin

**新增文档**:
- 下载并记录 XNF 论文 (Efficient LTLf Synthesis using Next Normal Form)
- 添加 XNF 转换规则详细说明文档
- 添加 DOT 图可视化调试指南

### 📊 Impact Analysis

- 修复了 `!p1` 测试用例，现在正确返回 REALIZABLE
- 改进了 Tableau 状态构造的语义正确性
- 为后续 XNF 转换实现打下基础

## Changes

### Added
- `docs/ARCHITECTURE/xnf_detailed.md`
- `docs/ARCHITECTURE/xnf_transformation.md`
- `docs/papers/xnf_paper.pdf`


### Modified
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **5** files changed
- **321** insertions(+)
- **141** deletions(-)
