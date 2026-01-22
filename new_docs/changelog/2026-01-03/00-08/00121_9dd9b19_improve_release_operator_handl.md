# [121] fix: improve Release operator handling and input dependency checks

**Commit**: `9dd9b19` ([`9dd9b19cf0bdbe91e7c9a69c6afcfa12bcb33335`](https://github.com/licoded/CosyZeroRewrite/commit/9dd9b19cf0bdbe91e7c9a69c6afcfa12bcb33335))
**Date**: 2026-01-03 00:18:18 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

This commit improves LTLf synthesis accuracy from 81.63% to 91.84%
by fixing several issues with Release operator and input dependency
handling.

Key changes:
1. Fixed Release operator semantics: left side having input
   dependencies is now allowed because system can choose to keep
   right side true forever
2. Fixed Until case to return false explicitly (was falling through
   to next case)
3. Added check for positive input literals in temporal states
4. Fixed has_negated_input_check for Release (only check right side)

Results:
- Accuracy: 81.63% → 91.84%
- False Negatives: 2 → 0
- False Positives: 7 → 4

Remaining issues (4 False Positives):
- f104, f114, f115, f129: complex nested formulas requiring
  deeper analysis

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
修复了 Release 算符的语义处理和输入依赖检查，将 LTLf synthesis 准确率从 81.63% 提升到 91.84%。主要解决了 False Negatives 问题，完全消除了此类错误。

### 🔍 Technical Details
**核心修复**:
1. **Release 算符语义**: Release `φ R ψ` 的语义是 "ψ 必须持续成立直到 φ 成立，如果 φ 永远不成立则 ψ 必须永远成立"。因此，左边 φ 有输入依赖是被允许的——系统可以选择让右边 ψ 永远成立。
2. **Until case 返回值**: 修复了 Until case 没有 `return false` 导致代码 fall through 到下一个 case 的 bug
3. **Temporal state 中的正输入字面量**: 添加了对正输入字面量（如 `input`）的检查，之前只检查了 `!input`
4. **has_negated_input_check for Release**: Release 只需要检查右边是否有 `!input`，左边的 `!input` 是允许的

**代码改动**:
- `src/automata/tableau.cpp`: 修改了 `is_accepting()` 函数中的 Release、Until、has_negated_input 等逻辑

### 📊 Impact Analysis
**测试结果** (49 个案例):
- 准确率: 81.63% → 91.84% (+10.21%)
- False Negatives: 2 → 0 ✅ (完全消除)
- False Positives: 7 → 4 (减少了 3 个)

**修复的案例**:
- f103, f106, f107: False Positives → 正确
- f112, f118: False Positives → 正确

**剩余问题** (4 个 False Positives):
- f104, f114, f115, f129: 这些是复杂的嵌套公式（Release 包含 Until/Until 包含输入字面量等），需要更深入的语义分析

## Changes

### Modified
- `src/automata/tableau.cpp`


## Stats

- **1** files changed
- **70** insertions(+)
- **32** deletions(-)
