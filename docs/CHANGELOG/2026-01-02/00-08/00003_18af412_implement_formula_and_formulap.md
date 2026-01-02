# [3] feat: implement Formula and FormulaPool

**Commit**: `18af412` ([`18af412ec5e5c0e672d0eb1fb881a43127ec4a27`](https://github.com/licoded/CosyZeroRewrite/commit/18af412ec5e5c0e672d0eb1fb881a43127ec4a27))
**Date**: 2026-01-02 01:04:12 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Formula: string representation, type predicates
- FormulaPool: hash consing, variable management, canonicalization
- Move semantics support

## AI Analysis

### 📝 Change Summary
实现 Formula 和 FormulaPool 的核心功能代码。FormulaPool 采用 **Hash Consing** 技术，通过 `unordered_set` 自动去重，确保结构相同的公式只存在唯一副本。配合移动语义优化，实现高效的公式创建和内存管理。

### 🔍 Technical Details

**Hash Consing 实现**：
```cpp
// FormulaPool 中的去重逻辑
auto result = pool_.insert(formula.get());
if (!result.second) {
    return static_cast<Formula*>(*result.first);  // 返回已存在的副本
}
return formula.release();  // 新公式，释放所有权
```

**关键优化**：
- **移动语义**：`create()` 方法返回 `Formula*`，调用方通过 `unique_ptr` 管理所有权
- **规范化输出**：变量按声明顺序分配 ID，确保 `to_string()` 输出的一致性
- **类型谓词**：`is_literal()`, `is_unary()`, `is_binary()` 快速判断公式结构

### 📊 Impact Analysis
- **范围**: `src/formula/formula.cpp`, `src/formula/formula_pool.cpp`
- **影响**: 所有公式操作的基础设施，影响解析、转换、求解等所有模块
- **性能**: Hash consing 消除重复公式，节省内存和后续操作时间

## Changes

### Added
- `src/formula/formula.cpp`
- `src/formula/formula_pool.cpp`

## Stats

- **2** files changed
- **534** insertions(+)
- **0** deletions(-)
