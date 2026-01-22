# [6] feat: implement formula simplification

**Commit**: `cff6ef4` ([`cff6ef42e53dc64ec2a87e2088a5f8a9745aa84f`](https://github.com/licoded/CosyZeroRewrite/commit/cff6ef42e53dc64ec2a87e2088a5f8a9745aa84f))
**Date**: 2026-01-02 01:04:54 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- O(n) HashSet-based deduplication (improved from O(n log n))
- Algebraic rules: a&True->a, a|False->a, etc.
- Until/Release specific optimizations
- Conflict detection for literals (a & !a -> False)

## AI Analysis

### 📝 Change Summary
实现公式化简模块，通过代数规则和冲突检测减少公式大小。使用 **HashSet** 实现 O(n) 去重，比原始 O(n log n) 实现更高效。化简在每次转换后执行，防止公式膨胀。

### 🔍 Technical Details

**化简规则示例**：
| 模式 | 化简结果 |
|------|---------|
| a ∧ True | a |
| a ∨ False | a |
| a ∧ !a | False |
| a ∨ a | a |
| (a ∧ b) ∨ (a ∧ c) | a ∧ (b ∨ c) |

**冲突检测**：
```cpp
// 检测 a ∧ !a 类型的冲突
for (auto lit : left_literals) {
    if (right_literals.count(negate(lit))) {
        return pool_->create_false();
    }
}
```

### 📊 Impact Analysis
- **范围**: `src/formula/simplify.cpp`
- **影响**: 每次转换 (NNF/XNF/rmnext) 后自动调用
- **性能**: O(n) HashSet 去重，显著减少中间公式大小

## Changes

### Added
- `src/formula/simplify.cpp`

## Stats

- **1** files changed
- **410** insertions(+)
- **0** deletions(-)
