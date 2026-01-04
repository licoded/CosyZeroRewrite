# [203] feat: optimize move enumeration using prop_atoms filtering

**Commit**: `fda894f` ([`fda894fa333bf98433449da66cc2e345c1a74950`](https://github.com/licoded/CosyZeroRewrite/commit/fda894fa333bf98433449da66cc2e345c1a74950))
**Date**: 2026-01-04 18:57:27 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add all_assignments_for_subset method to AssignmentGenerator
to generate assignments only for variables present in current
state's prop_atoms, reducing enumeration space from 2^n to 2^k.

Changes:
- Add AssignmentGenerator::all_assignments_for_subset()
- Modify expand_state() to extract relevant variables from prop_atoms
- Recursively extract literals from non-literal formulas (Next, Until, Release)

This optimization can significantly reduce edge enumeration space
especially when formulas have fewer variables than the total
declared in the partition file.

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
- `include/automata/tableau.hpp`
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **3** files changed
- **76** insertions(+)
- **4** deletions(-)
