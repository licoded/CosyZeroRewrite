# [18] Implement Bounded Model Checking (BMC) for LTLf in Z3 integration

**Commit**: `68f1692` ([`68f1692f33f25ed38ce046184de784dc2836e233`](https://github.com/licoded/CosyZeroRewrite/commit/68f1692f33f25ed38ce046184de784dc2836e233))
**Date**: 2026-01-02 01:53:19 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Uses time unrolling to handle temporal operators (U/R/X):
- Each proposition p becomes time-indexed: p_0, p_1, ..., p_bound
- X f at time t: f at time t+1
- f U g at time t: disjunction over all satisfaction points
- f R g at time t: g holds continuously until f becomes true

Bound selection strategies:
- max_bound = -1: incremental checking (start small, increase)
- max_bound = 0: auto-detect from formula structure
- max_bound > 0: use specified bound

Heuristics:
- X depth: count nested X operators (minimum bound needed)
- U/R depth: structural depth + X depth + 2

Fixed Release semantics: g holds continuously until f becomes true
(not just g OR f_held at each time point).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
实现 **Bounded Model Checking (BMC)**，通过时间展开将时态逻辑编码为命题逻辑序列。每个命题 `p` 变为 `p_0, p_1, ..., p_k`，时态算子转换为跨时间点的约束。

### 🔍 Technical Details

**时间展开编码**：
```
X f 在时间 t  ≡ f 在时间 t+1

f U g 在时间 t ≡ ⋁_{i=t..k} (g[i] ∧ ∀j∈[t,i).¬f[j])

f R g 在时间 t ≡ ∀j∈[t,k].(g[j] ∨ ∃i∈[t,j].f[i])
```

**边界选择策略**：
| max_bound | 行为 |
|-----------|------|
| -1 | 增量检查：从小界开始，失败后加倍 |
| 0 | 自动检测：X 深度 + U/R 深度 + 2 |
| > 0 | 使用指定边界 |

**Release 语义修正**：
原始实现将 `f R g` 编码为 `g ∨ f曾发生`，这是错误的。修正为：**g 持续保持直到 f 发生**。

### 📊 Impact Analysis
- **范围**: `src/formula/formula_z3.cpp`
- **影响**: 提供精确的 LTLf 公式等价性检查，支持任意公式
- **后续**: #9538eb5 实现增量 BMC 检查

## Changes

### Modified
- `include/formula/formula_z3.hpp`
- `src/formula/formula_checker.cpp`
- `src/formula/formula_z3.cpp`

## Stats

- **3** files changed
- **288** insertions(+)
- **145** deletions(-)
