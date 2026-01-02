# [91] docs: expand synthesis architecture with detailed class relationships

**Commit**: `ecad9e8` ([`ecad9e834d7065b454cf489bb1ae6f327715081c`](https://github.com/licoded/CosyZeroRewrite/commit/ecad9e834d7065b454cf489bb1ae6f327715081c))
**Date**: 2026-01-02 16:56:21 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added comprehensive documentation for:
- Complete data flow diagram from user input to result
- Why each component is necessary (FormulaPool, TableauState, OnTheFlyDFA, OnTheFlyGameSolver)
- OnTheFlyDFA adapter pattern deep dive:
  - Tableau vs Synthesis semantic differences
  - Adapter architecture diagram
  - Two-layer interface design (is_accepting vs is_tableau_accepting)
  - Implementation details with code examples
  - Comparison table for different formula types
  - Debugging scenarios using dual interfaces
- Data flow example for G(p0 → F p2)
- Class responsibility summary table

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
- `docs/ARCHITECTURE/synthesis.md`


## Stats

- **1** files changed
- **459** insertions(+)
- **0** deletions(-)
