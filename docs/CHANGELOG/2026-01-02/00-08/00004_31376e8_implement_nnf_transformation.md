# [4] feat: implement NNF transformation

**Commit**: `31376e8` ([`31376e81dfd38188b700dd21b493cd467197ef63`](https://github.com/licoded/CosyZeroRewrite/commit/31376e81dfd38188b700dd21b493cd467197ef63))
**Date**: 2026-01-02 01:04:48 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Push negations inward using De Morgan's laws
- Double negation elimination
- Temporal operator duality (U/R, F/G)
- LTLf Next negation with End marker: \!X(a) -> X(\!a) | End
- O(n) time complexity

## AI Analysis

### 📝 Change Summary
实现 LTLf 公式的 **Negation Normal Form (NNF)** 转换。核心思想是将否定算子推向叶子节点，使得否定仅出现在原子命题之前。这是 LTLf 求解流程的关键预处理步骤。

### 🔍 Technical Details

**LTLf 特殊处理**：
与 LTL 不同，LTLf 的 Next 算子否定需要考虑轨迹结束：
```
!X(a) ≡ X(!a) ∨ End
```
因为在有限轨迹中，"下一状态不满足 a" 可能是因为轨迹已结束。

**转换规则**：
| 原式 | NNF |
|------|-----|
| !(a ∧ b) | !a ∨ !b |
| !(a ∨ b) | !a ∧ !b |
| !X(a) | X(!a) ∨ End |
| !(a U b) | !a R !b |
| F(a) | true U a |
| G(a) | false R a |

### 📊 Impact Analysis
- **范围**: `src/formula/nnf.cpp`
- **影响**: Synthesis 流程的预处理阶段，所有公式在求解前必须先转为 NNF
- **复杂度**: O(n) 单次遍历

## Changes

### Added
- `src/formula/nnf.cpp`

## Stats

- **1** files changed
- **129** insertions(+)
- **0** deletions(-)
