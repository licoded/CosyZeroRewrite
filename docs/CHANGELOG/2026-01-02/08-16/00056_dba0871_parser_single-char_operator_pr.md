# [56] fix: parser single-char operator precedence bug (#002)

**Commit**: `dba0871` ([`dba0871f3bc5fceeaa7e1edcc63898369d12ed11`](https://github.com/licoded/CosyZeroRewrite/commit/dba0871f3bc5fceeaa7e1edcc63898369d12ed11))
**Date**: 2026-01-02 10:22:17 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Fixed bug where variables starting with 'r', 'f', or 'g' were
incorrectly parsed as Release/Finally/Globally operators.

Changes:
- Added peek-ahead logic in tokenizer to check if r/f/g is followed
  by another letter before treating it as an operator
- Added 5 regression tests covering r/f/g-prefixed variables
- Updated docs/BUGS/open.md (0 open bugs now)
- Updated docs/BUGS/fixed.md with bug #002 details
- Updated docs/TODO/parser.md to remove fixed item

Before: "req" → parsed as Release + "eq" (error)
After:  "req" → parsed as identifier "req" (correct)

Test results: 171 assertions in 36 test cases (was 124 in 31)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
修复解析器 **Bug #002**：以 'r', 'f', 'g' 开头的变量被错误识别为 Release/Finally/Globally 算子。根因是词法分析器缺乏**前瞻逻辑**，导致单字符算子优先于标识符匹配。

### 🔍 Technical Details

**Bug 原因**：
```cpp
// 错误逻辑：直接将 'r' 匹配为 Release
if (c == 'R' || c == 'r') {
    token.type = TokenType::Release;
}
```
导致 `"req"` → `Release + "eq"` (解析错误)

**修复方案 - Peek-Ahead**：
```cpp
// 正确逻辑：检查下一个字符
if ((c == 'R' || c == 'r') && i + 1 < input.size() &&
    std::isalpha(input[i + 1])) {
    // 标识符开头，继续读取
} else if (c == 'R' || c == 'r') {
    token.type = TokenType::Release;
}
```

**回归测试**：
添加 5 个测试用例覆盖 `request`, `fact`, `goal` 等前缀变量。

### 📊 Impact Analysis
- **范围**: `src/formula/formula_parser.cpp`, `tests/parser_checker_tests.cpp`
- **影响**: 解析器正确性，所有以 r/f/g 开头的变量名
- **测试**: 171 断言 (原 124)，全部通过

## Changes

### Modified
- `docs/BUGS/fixed.md`
- `docs/BUGS/open.md`
- `docs/TODO/parser.md`
- `src/formula/formula_parser.cpp`
- `tests/parser_checker_tests.cpp`

## Stats

- **5** files changed
- **224** insertions(+)
- **91** deletions(-)
