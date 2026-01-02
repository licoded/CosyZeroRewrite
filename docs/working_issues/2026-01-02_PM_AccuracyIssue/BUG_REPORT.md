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

## Debugging Methodology Lessons Learned

**CRITICAL LESSON (2026-01-02 14:10)**: Do NOT rely solely on final accuracy metrics to judge if a fix is correct.

When investigating systematic bugs:
1. ❌ **Wrong approach**: Make a change → run full benchmark → check accuracy
   - If accuracy drops, conclude the fix was wrong
   - This doesn't tell you WHY the fix is wrong

2. ✅ **Correct approach**:
   - Add debug logging to trace execution
   - Construct minimal test cases
   - Compare intermediate states with reference implementation
   - Verify each component independently

**Example of incorrect reasoning**:
- Modified Environment turn classification logic
- Ran benchmark: 55% accuracy (down from 64%)
- Concluded: "the fix was wrong"
- **Problem**: The original 64% might have been from OTHER bugs. The 55% might be moving in a different direction.

**Correct approach**:
- Verify the game theory semantics for Environment turn
- Compare with reference implementation's logic
- Test on individual formulas with debug output

## Investigation Steps

1. [ ] Compare SCC classification between Cosy and CosyZeroRewrite for specific failing formulas
2. [ ] Add debug output to trace classification through the algorithm
3. [ ] Review the propagation logic in `propagate_classification()`
4. [ ] Check if accepting state computation is correct
5. [ ] **[NEW]** Construct minimal test cases for each component
6. [ ] **[NEW]** Add detailed logging for f11 to trace execution path

## Debug Attempts Log

### Attempt 1: Environment turn classification (14:10)
- **Hypothesis**: Environment turn logic had `has_swin` instead of `all_swin`
- **Change**: Modified `propagate_classification()` lines 389-422
- **Result**: 55% accuracy (down from 64%)
- **Conclusion**: Inconclusive - need more systematic debugging
- **Status**: REVERTED

### Attempt 2: Temporal formula input dependency check (14:20)
- **Hypothesis**: Temporal formulas (Release, Until, Next) don't check input dependencies
- **Changes**:
  - Added check for Release formulas: if ψ requires input true, reject
  - Added check for Until formulas: if ψ requires input true, reject
  - Added check for Next formulas: if φ requires input true, reject
- **Test Results**:
  - G(p5) where p5 is input: PASS ✓ (was failing before)
  - G(p6) where p6 is output: PASS ✓
  - X(p5) where p5 is input: Still FAIL (Realizable, should be Unrealizable)
  - X(p6) where p6 is output: PASS ✓
- **Conclusion**: Partial fix - Release works, but Next still has issues
- **Root Cause Found**: Next literal evaluation path - when p5=false, state becomes {} which is accepting

### Attempt 3: Empty state handling (14:25)
- **Hypothesis**: Empty states should NOT be accepting for synthesis
- **Change**: Added check: if state is empty, return false from is_accepting()
- **Test Results**:
  - X(p5) where p5 is input: PASS ✓ (now correctly Unrealizable)
  - All simple tests: PASS ✓
  - Small benchmark (50 formulas): 55% accuracy (worse than 64%)
- **Conclusion**: Fix is too aggressive - breaks other cases
- **Status**: NEEDS REFINEMENT

### Key Findings

1. **Empty State Problem**: When p5=false from state {v0} (for X(p5)), the successor is {} (empty).
   - Empty state was marked as accepting
   - This allows Environment to "win" by making literals false
   - Fix: Reject empty states, but this breaks other formulas

2. **Until Formula Satisfaction**: Need to understand when φ U ψ is "done" and can end with empty state
   - If ψ is satisfied, the formula is complete
   - Empty state after satisfaction should be accepting
   - But empty state from unsatisfied Next should NOT be accepting

## Next Steps

1. [ ] Distinguish between "good empty" (formula satisfied) and "bad empty" (formula failed)
2. [ ] Track Next obligations through state transitions
3. [ ] Compare with Cosy reference implementation for Until/Next handling

## Files Modified

- `src/automata/tableau.cpp`:
  - Line 579-582: Added empty state rejection
  - Line 584-617: Added temporal formula input dependency checks
