# [11] Add FormulaChecker for equivalence and property checking

**Commit**: `7a86cfd` ([`7a86cfd068c0175a07b087ad2ab686d90e39f638`](https://github.com/licoded/CosyZeroRewrite/commit/7a86cfd068c0175a07b087ad2ab686d90e39f638))
**Date**: 2026-01-02 01:31:36 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implementation includes:
- are_equivalent(): Exact equivalence via truth table (≤4 vars)
- likely_equivalent(): Statistical equivalence via sampling (>4 vars)
- is_nnf(): Negation Normal Form property checker
- is_xnf(): neXt Normal Form property checker (allows U/R inside Next)
- Formula analysis: size, depth, variables, literals, primitives

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
实现公式验证和等价性检查工具。提供**精确** (真值表) 和**统计** (采样) 两种等价性检查策略，根据变量数量自动选择。同时实现 NNF/XNF 属性检查器，用于验证转换正确性。

### 🔍 Technical Details

**等价性检查策略**：
| 变量数 | 方法 | 复杂度 |
|--------|------|--------|
| ≤ 4 | 真值表穷举 | O(2^n) |
| > 4 | 随机采样 | O(k), k=样本数 |

**属性检查器**：
```cpp
bool is_nnf(Formula* f);
// 否定仅在原子命题前

bool is_xnf(Formula* f);
// U/R 不在 X 外部，即无 X(a U b) 形式
```

**公式分析**：
- `size()`: 节点总数
- `depth()`: 最大嵌套深度
- `variables()`: 变量集合
- `literals()`: 字面量集合

### 📊 Impact Analysis
- **范围**: `include/formula/formula_checker.hpp`, `src/formula/formula_checker.cpp`
- **影响**: 测试框架的基础，验证转换正确性
- **后续扩展**: #06d65ba 集成 Z3 支持多变量公式

## Changes

### Added
- `include/formula/formula_checker.hpp`
- `src/formula/formula_checker.cpp`

## Stats

- **2** files changed
- **560** insertions(+)
- **0** deletions(-)
