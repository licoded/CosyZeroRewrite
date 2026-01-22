# [116] feat(synthesis): add nested temporal and input dependency checks

**Commit**: `1f96877` ([`1f968777973cf63f2a1de8d2be39f6f2be789b6f`](https://github.com/licoded/CosyZeroRewrite/commit/1f968777973cf63f2a1de8d2be39f6f2be789b6f))
**Date**: 2026-01-02 22:19:41 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

This commit implements several improvements to LTLf synthesis:

1. **Nested temporal input dependency check**: Temporal formulas
   (Until/Release) nested inside Next operators are now checked for
   input dependencies. This fixes cases like G(input) where the
   Release formula is inside a Next after XNF transformation.

2. **Negated input literal check**: Temporal states now check for
   !input literals that would make the formula unrealizable.

3. **Initial state simplification**: OR/AND formulas with true/false
   are now simplified during initial state construction:
   - (true | ψ) → true
   - (false | ψ) → ψ
   - (true & ψ) → ψ
   - (false & ψ) → false

4. **Improved OR handling**: Temporal state checks now correctly
   handle OR formulas where only one side needs to be satisfied.

Current benchmark accuracy: 63.16% (small scale test)

Known issues:
- f100, f102, f105, f106, f107, f111: False Negatives due to
  OR formula expansion strategy (both sides added to state)
- f112: False Positive (p5 & F(p8) where p5 is input)

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
- **160** insertions(+)
- **14** deletions(-)
