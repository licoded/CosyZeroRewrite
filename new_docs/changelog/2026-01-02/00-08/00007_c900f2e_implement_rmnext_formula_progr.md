# [7] feat: implement rmnext formula progression

**Commit**: `c900f2e` ([`c900f2e52173f10ce8e69f247f9ad44e43e1c577`](https://github.com/licoded/CosyZeroRewrite/commit/c900f2e52173f10ce8e69f247f9ad44e43e1c577))
**Date**: 2026-01-02 01:04:57 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Progress formulas to next state given edge assignment
- Distribute over AND/OR with short-circuit
- Handle End marker for finite traces
- O(n) time complexity

## AI Analysis

### 📝 Change Summary
实现 **rmnext** 操作，给定边赋值将公式推进到下一状态。这是游戏求解中状态转移的核心：从当前状态的子公式集合，计算下一状态的子公式集合。

### 🔍 Technical Details

**语义转换**：
| 公式 | 下一状态 |
|------|---------|
| X(a) | a (直接展开) |
| a ∧ b | rmnext(a) ∧ rmnext(b) |
| a ∨ b | rmnext(a) ∨ rmnext(b) (短路求值) |
| End | False (轨迹结束) |

**短路优化**：
```cpp
// a ∨ True = True，无需计算 rmnext(b)
if (is_true(left)) return pool_->create_true();
```

**边赋值**：`edge` 是一个布尔映射，表示当前状态中各命题的真值。

### 📊 Impact Analysis
- **范围**: `src/formula/rmnext.cpp`
- **影响**: Tableau 构造和游戏求解的核心操作
- **复杂度**: O(n)，短路优化可减少实际计算量

## Changes

### Added
- `src/formula/rmnext.cpp`

## Stats

- **1** files changed
- **239** insertions(+)
- **0** deletions(-)
