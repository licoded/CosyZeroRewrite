# [8] test: integrate Catch2 testing framework

**Commit**: `2c25318` ([`2c253188d51394176eaf0fc5cffbd0c2374cb90e`](https://github.com/licoded/CosyZeroRewrite/commit/2c253188d51394176eaf0fc5cffbd0c2374cb90e))
**Date**: 2026-01-02 01:05:00 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add Catch2 v2.13.10 single-header
- 31 test cases with 66 assertions
- Organized by module: variable, formula, nnf, simplify, xnf, rmnext, integration
- All tests passing

## AI Analysis

### 📝 Change Summary
集成 **Catch2** 测试框架，建立完整的单元测试体系。采用单头文件模式 (v2.13.10)，无需额外依赖。测试覆盖所有核心模块：变量管理、公式操作、NNF/XNF/Simplify/rmnext 转换。

### 🔍 Technical Details

**测试组织结构**：
```cpp
TEST_CASE("Formula Pool", "[formula]") {
    SECTION("variable creation") { /* ... */ }
    SECTION("hash consing") { /* ... */ }
}
```

**初始测试覆盖**：
- 变量声明和查询
- Formula 类型谓词
- NNF 转换正确性
- Simplify 化简规则
- XNF 展开规则
- RmNext 状态推进

### 📊 Impact Analysis
- **范围**: `tests/catch2/`, `tests/formula_tests.cpp`
- **影响**: 建立回归测试基础，后续所有改动都需要通过这些测试
- **规模**: 31 测试用例，66 断言

`★ Insight ─────────────────────────────────────`
- **Catch2 v2 vs v3**: 选择 v2 因为单头文件模式更简单，不需要额外构建步骤
- **测试驱动开发**: 这些测试后续演化为 transformation_tests，验证 196 个公式的转换等价性
`─────────────────────────────────────────────────`

## Changes

### Added
- `tests/catch2/catch.hpp`
- `tests/formula_tests.cpp`

## Stats

- **2** files changed
- **18459** insertions(+)
- **0** deletions(-)
