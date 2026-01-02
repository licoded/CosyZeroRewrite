# Bug Report: Low Benchmark Accuracy (64% vs expected ~100%)

**Date**: 2026-01-02 14:05
**Severity**: Critical
**Status**: OPEN

## Problem Description

CosyZeroRewrite LTLf synthesis benchmark shows only **64% accuracy** on SMv1000 benchmark, while the reference Cosy implementation achieves **100% accuracy** on the same benchmark.

## Test Results

### Cosy Reference Implementation
```
Total test cases: 1000
Correct results: 1000
Incorrect results: 0
Accuracy: 100.00%
```

### CosyZeroRewrite
```
Total test cases: 458 (partial run)
Passed: 294 (64%)
Failed: 164 (36%)
True Positives: 249
True Negatives: 45
False Positives: 65
False Negatives: 99
Accuracy: 64.2%
```

## Error Pattern

The errors show a systematic pattern of **reversed results** (Realizable ↔ Unrealizable):

| Formula | Expected | Computed | Error Type |
|---------|----------|----------|------------|
| f11 | Realizable | Unrealizable | False Negative |
| f12 | Unrealizable | Realizable | False Positive |
| f15 | Unrealizable | Realizable | False Positive |
| f17 | Unrealizable | Realizable | False Positive |
| f18 | Realizable | Unrealizable | False Negative |
| f102 | Realizable | Unrealizable | False Negative |
| f104 | Unrealizable | Realizable | False Positive |

This pattern suggests a **fundamental algorithmic issue** rather than random errors.

## Potential Root Causes

1. **SCC Classification Logic** (`src/synthesis/on_the_fly_solver.cpp:304`)
   - The `try_classify_scc` function may have incorrect acceptance criteria
   - Swin/Ewin classification may be reversed

2. **I/O Separation Handling** (`src/automata/tableau.cpp:569`)
   - The `is_accepting` function handles non-temporal states with input/output separation
   - May be incorrectly rejecting valid accepting states

3. **Release Formula Semantics**
   - Previous bug fixes for Release formulas may be incomplete
   - LTLf Release semantics may not be fully captured

4. **Initial State Classification**
   - Final realizability check: `return result == StateClass::Swin;`
   - May need to check for different conditions

## Investigation Steps

1. [ ] Compare SCC classification between Cosy and CosyZeroRewrite for specific failing formulas
2. [ ] Add debug output to trace classification through the algorithm
3. [ ] Review the propagation logic in `propagate_classification()`
4. [ ] Check if accepting state computation is correct

## Files to Investigate

- `src/synthesis/on_the_fly_solver.cpp` - Main algorithm
- `src/automata/tableau.cpp` - DFA acceptance logic
- `include/automata/tableau.hpp` - State representation
