# [83] fix(synthesis): add temporal formula input dependency checks and debug

**Commit**: `1db430a` ([`1db430a572e109c5dd5b098b9997c6a5540b0f3e`](https://github.com/licoded/CosyZeroRewrite/commit/1db430a572e109c5dd5b098b9997c6a5540b0f3e))
**Date**: 2026-01-02 14:26:13 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Partially addresses the 64% benchmark accuracy issue by identifying and
fixing specific problems with temporal formulas:

1. Added input dependency checks for temporal formulas:
   - Release (false R p): reject if p requires input to be true
   - Until (φ U ψ): reject if ψ requires input to be true
   - Next (X φ): reject if φ requires input to be true

2. Identified empty state problem:
   - When Next literal is not satisfied, state becomes empty
   - Empty state was accepting, allowing Environment to "win"
   - Added check to reject empty states for synthesis

3. Test results after fixes:
   - G(p5) where p5 is input: now correctly Unrealizable ✓
   - X(p5) where p5 is input: now correctly Unrealizable ✓
   - Simple test cases: all pass ✓
   - Small benchmark (50 formulas): 55% accuracy

4. Known issues:
   - Empty state rejection is too aggressive
   - Need to distinguish "good empty" (formula satisfied) vs "bad empty" (failed)
   - Some Until formulas may be incorrectly rejected

Related: #82 - docs/working_issues/2026-01-02_PM_AccuracyIssue/BUG_REPORT.md

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
时态公式输入依赖检查：添加 Release、Until、Next 公式的输入依赖检查，部分修复 64% benchmark 准确率问题。

### 🔍 Technical Details

**修复的时态公式**：
- `false R p`: 如果 p 需要输入为真则拒绝
- `φ U ψ`: 如果 ψ 需要输入为真则拒绝
- `X φ`: 如果 φ 需要输入为真则拒绝

**发现的空状态问题**：
- 当 Next 字面量不满足时，状态变为空
- 空状态被标记为接受，允许环境"获胜"
- 添加检查在 synthesis 时拒绝空状态

**测试结果**：
- `G(p5)` (p5 输入): 现在正确 Unrealizable ✓
- `X(p5)` (p5 输入): 现在正确 Unrealizable ✓
- 小 benchmark (50 公式): 55% 准确率

**已知问题**：
- 空状态拒绝过于激进
- 需要区分"好空"(公式满足) vs "坏空"(失败)

## Changes

### Modified
- `docs/working_issues/2026-01-02_PM_AccuracyIssue/BUG_REPORT.md`
- `.gitignore`
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **4** files changed
- **141** insertions(+)
- **7** deletions(-)
