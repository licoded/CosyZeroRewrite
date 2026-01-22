# [221] feat: implement smart parentheses for formula string output

**Commit**: `9f37886` ([`9f378869f66d61e724e170c51e8f08674dd2d431`](https://github.com/licoded/CosyZeroRewrite/commit/9f378869f66d61e724e170c51e8f08674dd2d431))
**Date**: 2026-01-04 22:36:26 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implemented smart parentheses logic for Formula::to_string() and
to_string_with_names() to ensure clean, correct formula output
with proper roundtrip parsing behavior.

Changes:
- Added get_precedence() function matching Parser's hierarchy
- Added needs_parentheses() helper with clear rules
- Binary operators (And/Or/Until/Release) as children always get parens
- Next operator checks if child already has parentheses to avoid duplication
- Added is_wrapped_in_parens() helper for duplicate detection

括号规则 (2026-01-04):
- Literal/True/False: never need parentheses
- Not/Next: never need extra parentheses (Next adds its own X(...))
- Binary operators as children: always get parentheses for clarity
- Next checks child result to avoid duplicate parentheses

Test results:
- All 196 transformation formulas pass
- All 11 unit test suites pass

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
<!-- TODO: Add a brief summary of the change in Chinese or English -->

### 🔍 Technical Details
<!-- Optional: Add technical details, root cause, or implementation notes -->

### 📊 Impact Analysis
<!-- Optional: Add impact scope, affected components, or performance notes -->

## Changes

### Modified
- `CLAUDE.md`
- `src/formula/formula.cpp`


## Stats

- **2** files changed
- **176** insertions(+)
- **12** deletions(-)
