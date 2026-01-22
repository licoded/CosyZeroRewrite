# [178] fix: correct prop_atoms to exclude true/false constants

**Commit**: `844f4e0` ([`844f4e0fcb312f5c7f567548f2e87e1e683bef5e`](https://github.com/licoded/CosyZeroRewrite/commit/844f4e0fcb312f5c7f567548f2e87e1e683bef5e))
**Date**: 2026-01-04 12:07:07 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

PA(true) = {} and PA(false) = {} - constants are NOT atomic
propositions. The previous implementation incorrectly included true/false
in prop_atoms.

Changes:
- tableau.cpp: Return empty set for True/False in compute_prop_atoms()
- prop_atoms_test: Update tests to expect empty PA for true/false
- Update documentation comments

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
- `src/automata/tableau.cpp`
- `tests/integration/prop_atoms.cpp`


## Stats

- **2** files changed
- **21** insertions(+)
- **14** deletions(-)
