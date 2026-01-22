# [84] fix(synthesis): add temporal formula input dependency checks

**Commit**: `7b26038` ([`7b260389c4a6a93879687044012d1c92e275d25a`](https://github.com/licoded/CosyZeroRewrite/commit/7b260389c4a6a93879687044012d1c92e275d25a))
**Date**: 2026-01-02 14:44:35 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

This commit fixes several critical issues in the LTLf synthesis algorithm
that were causing benchmark accuracy to be only 64% instead of ~100%.

Changes:
1. Added Next formula input dependency check in is_accepting()
   - X(input) is now correctly UNREALIZABLE
2. Added Release formula input dependency check in is_accepting()
   - G(input) is now correctly UNREALIZABLE
3. Fixed terminal state classification to distinguish System vs Environment turn
4. Added handling for failed input literals in successor()
5. Added friend declaration for OnTheFlyDFA to access TableauState::formulas_

Test results (simple tests):
- X(p6) output: Realizable ✓
- X(p5) input: Unrealizable ✓ (was failing)
- G(p5) input: Unrealizable ✓ (was failing)
- G(p6) output: Realizable ✓

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
关键算法修复：完整修复时态公式输入依赖检查、终端状态分类和失败输入字面量处理，解决 64% benchmark 准确率问题的根本原因。

### 🔍 Technical Details

**核心修复**：
1. **Next 公式输入依赖检查** (`is_accepting()`):
   - `X(input)` 现在正确返回 UNREALIZABLE
   - Next 公式要求输入在下一状态为真时系统无法保证

2. **Release 公式输入依赖检查** (`is_accepting()`):
   - `G(input)` 现在正确返回 UNREALIZABLE
   - Release 公式右侧需要输入为真时系统无法保证

3. **终端状态分类** (`on_the_fly_solver.cpp`):
   - 区分 System/Environment 回合
   - System 无后继 → Ewin (无法移动)
   - Environment 无后继 → 接受时 Swin，否则 Ewin

4. **失败输入字面量处理** (`successor()`):
   - 输入字面量为 false 时添加 false 到下一状态
   - 防止"坏空"状态被标记为接受

**测试结果**：
- `X(p6)` 输出: Realizable ✓
- `X(p5)` 输入: **UNREALIZABLE** ✓ (之前失败)
- `G(p5)` 输入: **UNREALIZABLE** ✓ (之前失败)
- `G(p6)` 输出: Realizable ✓

## Changes

### Added
- `docs/working_issues/2026-01-02_PM_AccuracyIssue/CONTINUATION.md`


### Modified
- `CLAUDE.md`
- `docs/BUGS/open.md`
- `docs/working_issues/2026-01-02_PM_AccuracyIssue/BUG_REPORT.md`
- `include/automata/tableau.hpp`
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **7** files changed
- **393** insertions(+)
- **52** deletions(-)
