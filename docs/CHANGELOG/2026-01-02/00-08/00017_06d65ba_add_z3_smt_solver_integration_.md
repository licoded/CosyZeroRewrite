# [17] Add Z3 SMT solver integration for exact equivalence checking

**Commit**: `06d65ba` ([`06d65baafe811f439d1e5be8953716fa65edfcdc`](https://github.com/licoded/CosyZeroRewrite/commit/06d65baafe811f439d1e5be8953716fa65edfcdc))
**Date**: 2026-01-02 01:46:45 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Features:
- FormulaZ3 class with are_equivalent(), is_valid(), is_satisfiable()
- FormulaChecker::are_equivalent_smart() for automatic method selection
- Supports formulas with any number of variables (beyond 4-variable limit)
- Configurable timeout support (default 5000ms)
- CMake auto-detection for Z3 library with fallback stubs

Implementation details:
- Z3 headers included outside namespace to avoid std:: conflicts
- Uses pointer-based expression storage (z3::expr*) for unordered_map
- Single-point semantics for temporal operators (conservative approximation)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
集成 **Z3 SMT Solver**，提供精确的公式等价性检查，突破真值表法的变量数量限制 (4 个)。使用 **时间点语义** 将 LTLf 公式编码为命题逻辑，保守近似时态算子。

### 🔍 Technical Details

**Z3 集成架构**：
```cpp
class FormulaZ3 {
    z3::context ctx_;
    z3::solver solver_;

    bool are_equivalent(Formula* f1, Formula* f2);
    bool is_valid(Formula* f);      // f ≡ true
    bool is_satisfiable(Formula* f); // ∃赋值使 f 为真
};
```

**智能方法选择** (`are_equivalent_smart`)：
```
变量 ≤ 4   → 真值表 (快)
变量 > 4   → Z3 (精确)
```

**时态算子编码** (单点语义)：
- `X(a) ≡ a` (保守: 下一状态 ≡ 当前状态)
- `F(a) ≡ a` (保守: 最终 ≡ 现在)
- `G(a) ≡ a` (保守: 始终 ≡ 现在)

### 📊 Impact Analysis
- **范围**: `include/formula/formula_z3.hpp`, `src/formula/formula_z3.cpp`
- **影响**: 支持任意数量变量的等价性检查，BMC 基础
- **依赖**: Z3 库 (可选，无 Z3 时使用 stub)

`★ Insight ─────────────────────────────────────`
- **CMake 自动检测**: `find_package(Z3)` 失败时使用 stub 实现，保持代码可编译
- **单点语义局限**: 后续 #68f1692 通过 BMC 解决时态算子编码问题
`─────────────────────────────────────────────────`

## Changes

### Added
- `include/formula/formula_z3.hpp`
- `src/formula/formula_z3.cpp`

### Modified
- `CMakeLists.txt`
- `include/formula/formula_checker.hpp`
- `src/formula/formula_checker.cpp`

## Stats

- **5** files changed
- **466** insertions(+)
- **0** deletions(-)
