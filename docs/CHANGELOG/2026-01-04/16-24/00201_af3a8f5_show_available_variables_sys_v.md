# [201] feat: show available variables (sys_vars, env_vars) in state tooltip

**Commit**: `af3a8f5` ([`af3a8f5ac46334d9830fb80d9a18afe2a8cef1bf`](https://github.com/licoded/CosyZeroRewrite/commit/af3a8f5ac46334d9830fb80d9a18afe2a8cef1bf))
**Date**: 2026-01-04 17:22:41 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Changes:
- Add sys vars and env vars display in node tooltip
- Extract literal variable names from prop_atoms
- Use partition.outputs/inputs to distinguish sys vs env vars
- Show format: "sys vars = {p}" or "env vars = {}"

Example display for S0 state:
  Type: System
  sys vars = {p}
  env vars = {}

Data source:
- prop_atoms: contains literals and subformulas
- partition: distinguishes outputs (sys) vs inputs (env)
- extractLiterals(): filters out X(...), |, &, ( patterns

Related: docs/working_issues/2026-01-04_PM_StateTooltipVars/DESIGN.md

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

### Added
- `docs/working_issues/2026-01-04_PM_StateTooltipVars/DESIGN.md`


### Modified
- `visualization/src/components/GraphCanvas.vue`


## Stats

- **2** files changed
- **183** insertions(+)
- **1** deletions(-)
