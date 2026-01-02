# [120] fix: improve temporal dependency checking for nested formulas

**Commit**: `cfcd0da` ([`cfcd0da95958c529d8a2a0f942dc5e8465af3227`](https://github.com/licoded/CosyZeroRewrite/commit/cfcd0da95958c529d8a2a0f942dc5e8465af3227))
**Date**: 2026-01-02 23:52:35 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Fixed a critical bug where `is_temporal()` only checked the top-level
operator, missing temporal operators nested inside other operators
(like OR containing Next).

Changes:
- Added recursive `has_nested_temporal()` to detect nested temporal ops
- Extended `requires_input_true()` to handle Until/Release/Next
- Changed `requires_input_true()` for NOT to return true for !input
- Updated Until/Release checks to examine both sides

Results:
- Accuracy: 78.95% → 81.63% (+2.68%)
- Fixed False Positives: f103, f114, f115, f137 (4 cases)
- New False Negatives: 2 (needs investigation)

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
- `tests/tableau_state_test.cpp`


## Stats

- **2** files changed
- **141** insertions(+)
- **17** deletions(-)
