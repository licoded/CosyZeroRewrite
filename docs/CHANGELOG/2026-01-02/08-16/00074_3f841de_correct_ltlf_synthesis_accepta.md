# [74] fix: correct LTLf synthesis acceptance conditions for Release and I/O separation

**Commit**: `3f841de` ([`3f841de9e050796f852d3318ae947db8098925f6`](https://github.com/licoded/CosyZeroRewrite/commit/3f841de9e050796f852d3318ae947db8098925f6))
**Date**: 2026-01-02 13:06:05 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

This commit fixes multiple critical bugs in the LTLf synthesis solver that
caused incorrect realizability classifications.

Bug #4: Non-temporal state acceptance with input/output separation
- Added OnTheFlyDFA::num_outputs_ to track output variable count
- Modified OnTheFlyDFA::is_accepting() to check if non-temporal states
  can be satisfied using only output variables
- Added requires_input_true() helper to detect formulas requiring inputs
- Fix: p1 & q1 now correctly returns Unrealizable (q1 is input)

Bug #5: Release formula acceptance for LTLf finite trace semantics
- Modified is_locally_consistent() to allow false when it's part of
  a Release formula (false R φ) structure, which represents G(φ)
- Modified is_accepting() to treat Release-only states as accepting
  when all right sides are satisfied (finite trace termination)
- Made is_temporal() public for use in OnTheFlyDFA
- Fixes: G(p1), F(X(p1)), and complex Until formulas now work correctly

Test Results:
- p1 & q1: Realizable -> Unrealizable ✓ (correct)
- G(p1): Unrealizable -> Realizable ✓ (correct)
- F(X(p1)): Unrealizable -> Realizable ✓ (correct)
- (X(F(p1))) U (X(X(G(p1)))): Unrealizable -> Realizable ✓ (correct)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
关键 bug 修复：修复 LTLf synthesis 中 Release 公式和 I/O 分离的接受条件，解决了多个错误分类问题。

### 🔍 Technical Details

**Bug #4: 非时态状态的 I/O 分离接受检查**
- 添加 `OnTheFlyDFA::num_outputs_` 追踪输出变量数
- 修改 `is_accepting()` 检查非时态状态能否仅用输出满足
- 添加 `requires_input_true()` 辅助函数检测公式是否需要输入

**Bug #5: Release 公式的 LTLf 有限轨迹语义**
- 修改 `is_locally_consistent()` 允许 `false` 在 Release 结构中
- 修改 `is_accepting()` 将仅含 Release 的状态视为接受
- 使 `is_temporal()` 公开供 OnTheFlyDFA 使用

**测试结果**：
- `p1 & q1`: Realizable → Unrealizable ✓ (正确，q1 是输入)
- `G(p1)`: Unrealizable → Realizable ✓ (正确)
- `F(X(p1))`: Unrealizable → Realizable ✓ (正确)

## Changes

### Modified
- `include/automata/tableau.hpp`
- `include/synthesis/on_the_fly_solver.hpp`
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **4** files changed
- **344** insertions(+)
- **23** deletions(-)
