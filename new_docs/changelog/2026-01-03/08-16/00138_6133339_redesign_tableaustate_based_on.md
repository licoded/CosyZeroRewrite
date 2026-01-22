# [138] refactor: redesign TableauState based on AAAI2019 PA definition

**Commit**: `6133339` ([`6133339cff01ab33e3a59ba24cb1a6ba4e4fb0e0`](https://github.com/licoded/CosyZeroRewrite/commit/6133339cff01ab33e3a59ba24cb1a6ba4e4fb0e0))
**Date**: 2026-01-03 10:47:56 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Major architectural changes to TableauState:
- formulas_ → prop_atoms_ (Propositional Atoms per AAAI2019 Def 2)
- Add phi_ (original formula, used for hash and empty-string check)
- Add xnf_phi_ (XNF form, used for formula progression)
- Hash now based only on phi_ (simpler and more canonical)

Also:
- Remove is_accepting() methods from TableauState/OnTheFlyDFA
- Update TableauStatePool::get_or_create to take phi
- Remove num_outputs_ from OnTheFlyDFA

Reference: AAAI2019 - De Giacomo et al., Definition 2 (PA)

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


## Stats

- **1** files changed
- **60** insertions(+)
- **105** deletions(-)
