# [2] feat: implement Formula class with immutable design

**Commit**: `d10c97b` ([`d10c97b30aec138975f62d240fff8effae64ef51`](https://github.com/licoded/CosyZeroRewrite/commit/d10c97b30aec138975f62d240fff8effae64ef51))
**Date**: 2026-01-02 01:04:10 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- OpType enum for LTLf operators
- Immutable formula representation
- Cached hash for efficient comparisons
- Virtual methods for transformations (nnf, simplify, xnf, rmnext)

## AI Analysis

### 📝 Change Summary
实现 LTLf 公式的不可变 AST 表示。Formula 类采用**不可变设计**，所有字段在构造后不可修改，配合 FormulaPool 实现 **Hash Consing**（结构去重）。这是整个公式模块的核心基础。

### 🔍 Technical Details

**设计决策**：
- **原始指针而非 shared_ptr**：避免引用计数开销，由 FormulaPool 统一管理生命周期
- **缓存哈希值**：构造时计算，支持 O(1) 的相等性比较和去重
- **OpType 枚举**：包含 LTLf 完整算子集（True/False/Not/And/Or/Next/Until/Release/End/Literal）
- **虚拟方法接口**：为后续 NNF、XNF、Simplify 等转换预留扩展点

**类图结构**：
```mermaid
classDiagram
    class Formula {
        +OpType: OpType
        +left_: Formula*
        +right_: Formula*
        +hash_: size_t
        +op() OpType
        +left() Formula*
        +right() Formula*
        +is_literal() bool
        +is_unary() bool
        +is_binary() bool
        +accept(Visitor) void
    }
    class FormulaPool {
        -pool_: unordered_set
        -variables_: vector
        +create(OpType, ...) Formula*
        +get_variable(id) string
    }
    FormulaPool "1" -- "*" creates --> Formula
```

### 📊 Impact Analysis
- **范围**: `include/formula/formula.hpp`, `include/formula/formula_pool.hpp`
- **影响**: 所有后续公式操作的基础，影响 NNF、XNF、Simplify、Synthesis 等所有模块
- **性能**: Hash consing 消除重复公式，节省内存和计算时间

### ⚠️ Notes
- Formula 构造函数为 private，必须通过 FormulaPool 创建
- 不可变设计意味着修改公式会产生新对象，需注意指针更新

## Changes

### Added
- `include/formula/formula.hpp`
- `include/formula/formula_pool.hpp`

## Stats

- **2** files changed
- **551** insertions(+)
- **0** deletions(-)
