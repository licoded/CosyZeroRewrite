# [92] refactor: remove redundant set_variables() call in parser

**Commit**: `7578900` ([`7578900041aa2ec992159874ac2e889f96c2a600`](https://github.com/licoded/CosyZeroRewrite/commit/7578900041aa2ec992159874ac2e889f96c2a600))
**Date**: 2026-01-02 17:24:52 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

FormulaParser now auto-loads variable names from FormulaPool in constructor,
eliminating the need for manual set_variables() call after declare_variables().

Changes:
- FormulaPool: Add get_all_variable_names() method
- FormulaParser: Auto-load variables from pool in constructor
- cosy2.cpp: Remove redundant set_variables() call

Before:
  pool.declare_variables(outputs, inputs);
  parser.set_variables(all_vars);  // ← Redundant!

After:
  pool.declare_variables(outputs, inputs);
  FormulaParser parser(pool);  // Auto-loads from pool

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
- `include/formula/formula_pool.hpp`
- `src/cosy2.cpp`
- `src/formula/formula_parser.cpp`


## Stats

- **3** files changed
- **15** insertions(+)
- **7** deletions(-)
