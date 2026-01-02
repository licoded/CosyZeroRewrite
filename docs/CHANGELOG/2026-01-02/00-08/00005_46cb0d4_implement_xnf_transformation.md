# [5] feat: implement XNF transformation

**Commit**: `46cb0d4` ([`46cb0d4b791375d2d2ec423724e7502dc83a362e`](https://github.com/licoded/CosyZeroRewrite/commit/46cb0d4b791375d2d2ec423724e7502dc83a362e))
**Date**: 2026-01-02 01:04:51 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Expand Until/Release to top level
- Eventually true (♢true) = !End
- Always false (□false) = End
- No recursion on U/R inside Next (handled by rmnext)
- O(n) single-pass expansion

## AI Analysis

### 📝 Change Summary
实现 **XNF (eXtended Normal Form)** 转换，将 Until/Release 算子展开到公式顶层。这是为 rmnext 做准备的关键步骤，使得 X 算子仅包裹原子公式。

### 🔍 Technical Details

**核心转换规则**：
```
X(a U b) → X(a U b)          # X 内的 U 不展开
X(a) U b → (X(a) ∨ b) U b    # 顶层展开
```

**LTLf 特殊简化**：
```
♢true ≡ !End     # "最终为真" = "轨迹未结束"
□false ≡ End     # "始终为假" = "轨迹结束"
```

这些简化利用了 LTLf 的有限轨迹特性，大幅简化后续计算。

### 📊 Impact Analysis
- **范围**: `src/formula/xnf.cpp`
- **影响**: NNF 之后、rmnext 之前必须执行
- **复杂度**: O(n) 单次遍历，无递归

## Changes

### Added
- `src/formula/xnf.cpp`

## Stats

- **1** files changed
- **119** insertions(+)
- **0** deletions(-)
