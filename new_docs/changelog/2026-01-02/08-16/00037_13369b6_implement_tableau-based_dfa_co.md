# [37] feat: implement tableau-based DFA construction

**Commit**: `13369b6` ([`13369b66541a989d2e59bbcf8c4acb463f39fc80`](https://github.com/licoded/CosyZeroRewrite/commit/13369b66541a989d2e59bbcf8c4acb463f39fc80))
**Date**: 2026-01-02 09:13:54 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implement on-the-fly DFA construction using tableau states as sets
of subformulas, following Algorithm 1 from arXiv:2408.07324.

- TableauState: DFA state as set of formulas with hash consing
- TableauStatePool: State deduplication and memory management
- OnTheFlyDFA: Lazy transition computation with caching
- AssignmentGenerator: Enumerate all variable assignments

Key features:
- Local consistency checking (contradiction detection)
- Proper handling of Release formulas (keep both right side and formula)
- Until formula continuation when right side not satisfied
- Next formula unwrapping

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
实现基于 Tableau 的 LTLf → DFA 转换，遵循 arXiv:2408.07324 Algorithm 1。核心思想是将 DFA 状态表示为子公式集合，而非传统的显式状态枚举。这种方法天然支持 **on-the-fly** 展开，避免构造完整 DFA。

### 🔍 Technical Details

**Tableau 状态结构**：
```cpp
class TableauState {
    FormulaSet formulas;  // 子公式集合
    bool is_accepting();  // 接受条件判定
};
```

**局部一致性规则**（Tableau 1）：
- 不能同时包含 `a` 和 `!a`
- 若包含 `a ∧ b`，必须包含 `a` 和 `b`
- 若包含 `a ∨ b`，必须包含 `a` 或 `b`
- 若包含 `a U b`，必须包含 `b` 或同时包含 `a`

**下一状态计算**（Tableau 2）：
对每个赋值 `π`，`next(Γ, π)` 返回所有 X 公式展开后的子公式集合。

### 📊 Impact Analysis
- **范围**: `include/automata/dfa.hpp`, `include/automata/tableau.hpp`, `src/automata/`
- **影响**: Synthesis 流程的核心，将 LTLf 公式转换为游戏图
- **性能**: Hash consing + Lazy evaluation，按需展开状态

## Changes

### Added
- `include/automata/dfa.hpp`
- `include/automata/tableau.hpp`
- `src/automata/dfa.cpp`
- `src/automata/tableau.cpp`

## Stats

- **4** files changed
- **1804** insertions(+)
- **0** deletions(-)
