# Remaining False Positives in LTLf Synthesis

**Date**: 2026-01-03 00:20
**Status**: OPEN
**Priority**: MEDIUM
**Accuracy**: 91.84% (45/49 passed, 4 failed)

## Problem Description

After fixing Release operator handling, 4 False Positives remain:
- **f104**: Expected U, Got R
- **f114**: Expected U, Got R
- **f115**: Expected U, Got R
- **f129**: Expected U, Got R

All are complex nested formulas where our implementation incorrectly returns REALIZABLE when the correct answer is UNREALIZABLE.

## Formulas Analysis

### f104
```
Formula: (p7) R (((!(p3)) R (X(!(p4)))) U (p0))
Inputs: p0, p3 (environment)
Outputs: p4, p7 (system)
Parsed: (v1 R ((!v3 R X(!v0)) U v2))
Expected: UNREALIZABLE
Got: REALIZABLE
```

**Issue**: Outer Release with Until inside. The Until `((!p3) R (X(!p4))) U (p0)` contains p0 (input) on the right side. Our check detects this and rejects, but the game solver might still find a winning strategy.

### f114
```
Formula: F((X(X(G(!(p8))))) R ((p3) & (F(p4)) & ((p6) U (G(p5)))))
Inputs: p3, p4 (environment)
Outputs: p5, p6, p8 (system)
Expected: UNREALIZABLE
Got: REALIZABLE
```

**Issue**: F(...) wrapping a Release. Inside the Release: `((p3) & (F(p4)) & ((p6) U (G(p5))))`. This contains p3, p4 which are inputs in AND context - system cannot guarantee them.

### f115
```
Formula: (X(X(p3))) R ((G(p4)) U (p0))
Inputs: p0, p3 (environment)
Outputs: p4 (system)
Expected: UNREALIZABLE
Got: REALIZABLE
```

**Issue**: Release with left side `X(X(p3))` where p3 is input. Our check allows this (correct for Release), but the right side `G(p4) U p0` contains p0 (input) on the Until's right.

### f129
```
Formula: F((F(G(p4))) & (G((p5) & ((p1) | (!(p5))))))
Inputs: p1, p4 (environment)
Outputs: p5 (system)
Expected: UNREALIZABLE
Got: REALIZABLE
```

**Issue**: Complex formula with G(p4) where p4 is input. Globally requires input to be true forever, which system cannot guarantee.

## Root Cause Analysis

These cases share common patterns:

1. **Nested temporal operators**: Release containing Until, F containing Release, etc.
2. **Inputs in AND contexts**: When an input appears in a conjunctive requirement (AND or Until right side), the system cannot guarantee satisfaction
3. **Globally with inputs**: `G(input)` is always unrealizable (system can't control input forever)

## Current Checks

Our `is_accepting()` function checks:
- `!input` in temporal states
- `input` (positive literal) in temporal states
- Release right side for input dependencies
- Until both sides for input dependencies

**Potential gaps**:
1. Nested temporal formulas might not be fully explored
2. G(input) patterns need explicit checking
3. AND formulas inside temporal contexts need careful handling

## Next Steps

1. Add explicit check for `G(input)` patterns
2. Verify recursive checking explores all nested formulas
3. Consider marking formulas with deep input dependencies as always non-accepting

## Verification with Cosy

All 4 formulas confirmed as UNREALIZABLE by Cosy reference implementation.

## Test Commands

```bash
# Test individual formulas
./build/Cosy2 -f benchmarks/sm1000/bench1/f104.ltlf -p benchmarks/sm1000/bench1/f104.part
./build/Cosy2 -f benchmarks/sm1000/bench1/f114.ltlf -p benchmarks/sm1000/bench1/f114.part
./build/Cosy2 -f benchmarks/sm1000/bench1/f115.ltlf -p benchmarks/sm1000/bench1/f115.part
./build/Cosy2 -f benchmarks/sm1000/bench1/f129.ltlf -p benchmarks/sm1000/bench1/f129.part

# Verify with Cosy
Cosy /tmp/test_f104.ltlf /tmp/test_f104.part 0
```

## Related Files

- `src/automata/tableau.cpp`: `OnTheFlyDFA::is_accepting()` function
- `benchmarks/sm1000/bench1/f104.ltlf`, `f114.ltlf`, `f115.ltlf`, `f129.ltlf`
