# [109] fix: improve initial_state construction and add XNF documentation

**Commit**: `cdd1cae` ([`cdd1cae4be6a8df16792deb5c4ec668782339b37`](https://github.com/licoded/CosyZeroRewrite/commit/cdd1cae4be6a8df16792deb5c4ec668782339b37))
**Date**: 2026-01-02 21:29:48 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Fix: Don't expand child of Not operators in initial_state (fixes !p1 case)
- Fix: Empty DFA states now correctly marked as accepting
- Fix: Terminal System states now check is_accepting for classification
- Add: XNF paper and transformation documentation
- Add: DOT graph visualization guide for debugging

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
- `docs/ARCHITECTURE/xnf_detailed.md`
- `docs/ARCHITECTURE/xnf_transformation.md`
- `docs/papers/xnf_paper.pdf`


### Modified
- `src/automata/tableau.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **5** files changed
- **321** insertions(+)
- **141** deletions(-)
