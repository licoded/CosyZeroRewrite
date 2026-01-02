# [10] Add FormulaParser for LTLf formula string parsing

**Commit**: `2284eb7` ([`2284eb77ed9f9c371fa89ba5aaeb03f82bd875db`](https://github.com/licoded/CosyZeroRewrite/commit/2284eb77ed9f9c371fa89ba5aaeb03f82bd875db))
**Date**: 2026-01-02 01:31:32 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implement recursive descent parser supporting:
- Boolean operators: ! (not), & (and), | (or)
- Temporal operators: X (next), U (until), R (release)
- Parentheses for grouping
- Constants: true, false
- Auto-declaration of single-char variables
- Multi-char variables via set_variables()

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
实现 **递归下降解析器**，支持完整 LTLf 算子集合。采用**运算符优先级**解析策略，优先级从高到低：`!` > `&` > `|` > `U/R` > `X`。单字符变量自动声明机制简化使用。

### 🔍 Technical Details

**语法设计**：
```
formula   ::= implies
implies   ::= or ('->' or)?
or        ::= and ('|' and)*
and       ::= unary ('&' unary)*
unary     ::= '!' unary | primary
primary   ::= '(' formula ')' | temporal | literal
temporal  ::= ('X' | 'F' | 'G') primary | primary ('U' | 'R') primary
```

**变量声明策略**：
- 单字符变量 (a-z): 自动声明，无需预处理
- 多字符变量: 需先调用 `set_variables()` 声明

### 📊 Impact Analysis
- **范围**: `include/formula/formula_parser.hpp`, `src/formula/formula_parser.cpp`
- **影响**: 提供公式字符串 → AST 的转换，是用户交互的核心接口
- **已知问题**: r/f/g 前缀变量解析错误 (后续在 #dba0871 修复)

## Changes

### Added
- `include/formula/formula_parser.hpp`
- `src/formula/formula_parser.cpp`

## Stats

- **2** files changed
- **488** insertions(+)
- **0** deletions(-)
