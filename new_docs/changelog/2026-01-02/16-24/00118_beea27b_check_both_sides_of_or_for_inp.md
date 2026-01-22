# [118] feat(synthesis): check both sides of OR for !input

**Commit**: `beea27b` ([`beea27b1f899b9f7cc9cdc75633526cf38d975a6`](https://github.com/licoded/CosyZeroRewrite/commit/beea27b1f899b9f7cc9cdc75633526cf38d975a6))
**Date**: 2026-01-02 23:33:36 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Modified `has_negated_input` to check if BOTH sides of an OR
contain !input (only then reject). If only one side has !input,
the other side might be satisfiable.

This is an incremental improvement. The remaining False Positives
(f103, f104, f114, f115) have a different issue: one side requires
input=true (G(input)) and the other requires input=false (!input).
Current check only handles !input, not "requires input=true".

Current accuracy: 78.95% (15/19 passed)

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


## Stats

- **1** files changed
- **6** insertions(+)
- **3** deletions(-)
