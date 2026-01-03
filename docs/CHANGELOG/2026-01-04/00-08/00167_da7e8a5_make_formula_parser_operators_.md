# [167] fix: make formula parser operators case-sensitive (uppercase only)

**Commit**: `da7e8a5` ([`da7e8a53cc0184c66769f3caed2f7bc716698f73`](https://github.com/licoded/CosyZeroRewrite/commit/da7e8a53cc0184c66769f3caed2f7bc716698f73))
**Date**: 2026-01-04 01:06:30 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Only uppercase X, U, R, F, G are now recognized as LTL operators.
Lowercase x, u, r, f, g can be used as variable names.

Changes:
- Removed lowercase 'x' and 'u' from single-char operator cases
- Removed lowercase 'r', 'f', 'g' from ambiguous operator checks
- Made F/G keyword matching case-sensitive (removed tolower)
- Added parser integration tests for case sensitivity

Fixes prop_atoms_test parser integration tests.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary

修复 FormulaParser 对运算符的大小写敏感性，现在只有大写的 X, U, R, F, G 被识别为 LTL 时序运算符，小写字母可以用作变量名。

### 🔍 Technical Details

**问题根因**: 原词法分析器将小写 'x', 'u', 'r', 'f', 'g' 也识别为运算符，导致无法使用这些字母作为变量名。

**修改位置**: `src/formula/formula_parser.cpp:tokenize()`

**关键改动**:
1. 单字符运算符 case 只保留大写 X 和 U
2. 移除 'r', 'f', 'g' 的小写分支，只保留大写
3. 移除关键字比较的 tolower() 转换，直接比较 "F" 和 "G"

### 📊 Impact Analysis

- **影响范围**: 所有使用 FormulaParser 的公式解析
- **测试覆盖**: prop_atoms_test 新增 3 个 parser 集成测试
- **兼容性**: 现有公式使用大写运算符，无破坏性变更

## Changes

### Modified
- `src/formula/formula_parser.cpp`
- `tests/prop_atoms_test.cpp`


## Stats

- **2** files changed
- **69** insertions(+)
- **22** deletions(-)
