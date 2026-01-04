# XNF Transformation Algorithm

**Date**: 2025-01-01
**Status**: Design Complete, Ready for Implementation

---

## Table of Contents

1. [Introduction](#introduction)
2. [Theoretical Foundation](#theoretical-foundation)
3. [Key Concepts](#key-concepts)
4. [XNF Transformation Rules](#xnf-transformation-rules)
5. [Implementation](#implementation)
6. [Examples](#examples)
7. [Integration with NNF](#integration-with-nnf)

---

## Introduction

**XNF (neXt Normal Form)**: A formula is in XNF if its `pa(φ)` (primitive subformulas) only includes literals, Strong Next (◦/X), and Weak Next (•/WX).

**Goal**:
- Separate "current state constraints" from "next state constraints"
- Prepare for DFA construction via rmnext progression
- Eliminate Until/Release operators from primitive subformulas

**Transformation Pipeline**:
```
Parse → NNF → XNF → (rmnext during DFA construction)
```

---

## Theoretical Foundation

### Primitive Subformulas (pa)

**Definition**: For an LTLf formula φ in NNF, `pa(φ)` is the set of literals and temporal subformulas whose primary connective is temporal.

**Formal Definition**:
```
pa(φ) = {φ}              if φ is a literal or temporal formula
pa(φ) = pa(φ1) ∪ pa(φ2) if φ = (φ1 ∧ φ2) or φ = (φ1 ∨ φ2)
```

**Examples**:
```
pa(a)           = {a}
pa(X(a))        = {X(a)}
pa(a ∧ X(b))    = {a, X(b)}
pa(a U b)       = {a U b}           # Until is temporal!
pa((a ∧ b) U c) = {(a ∧ b) U c}     # The whole Until formula
```

### XNF Definition

**Definition**: An LTLf formula φ is in **neXt Normal Form (XNF)** if `pa(φ)` only includes:
- Literals (atomic variables or negated variables)
- Strong Next formulas (X/◦)
- Weak Next formulas (WX/•)

**In other words**: `pa(φ)` does NOT contain Until (U) or Release (R) formulas.

---

## Key Concepts

### End / Tail Marker

**End** (or **Last**): A special atomic proposition marking the final position in a finite trace.

**Semantics**:
```
π, i ⊨ End  iff  i is the last position of trace π
```

**Role in LTLf**:
- **Strong Next (X)**: Requires existence of next position
  - At End: `X(φ)` is **False** (no next position exists)

- **Weak Next (WX)**: Does NOT require next position
  - At End: `WX(φ)` is **True** (vacuously satisfied)

**Implementation**:
- Treat `End` as a special atomic proposition (similar to True/False)
- Created by `FormulaPool::create_end()`
- During rmnext: `End → False` (no next state)

### Eventually True / Always False

**Definitions**:
```
♢true  ≡ true U true   (eventually true)
□false ≡ false R false (always false)
```

**LTLf Semantics with End**:
```
♢true  ≡ ¬End   (not at the end = eventually holds)
□false ≡ End    (at the end = always false)
```

**Purpose in XNF**:
- `♢true` guarantees U-formulas do NOT accept empty traces
- `□false` guarantees R-formulas DO accept empty traces

### Tail vs End

**Tail**: The remaining subtrace starting from the next position.

Given trace `π = σ₀, σ₁, ..., σₙ`:
- **Head**: `σ₀` (current position)
- **Tail**: `σ₁, σ₂, ..., σₙ` (remaining positions)

**Connection to X**:
- `X(φ)` means "φ holds in Tail"
- `X` operator is essentially checking Tail

**In Examples**:
```
φ = (¬Tail ∧ a) U b
```
Here, `¬Tail` = `¬End` = "not at the last position" = "there is a next position"

---

## XNF Transformation Rules

Based on (Li et al. 2019), for LTLf formula φ in NNF:

### Base Cases (Already in XNF)
```
xnf(φ) = φ  if φ is:
  - literal (a or ¬a)
  - □false (always false)
  - ♢true (eventually true)
  - ◦-formula (Strong Next: X(φ))
  - •-formula (Weak Next: WX(φ))
```

### Boolean Connectives (Distribute)
```
xnf(φ₁ ∧ φ₂) = xnf(φ₁) ∧ xnf(φ₂)
xnf(φ₁ ∨ φ₂) = xnf(φ₁) ∨ xnf(φ₂)
```

### Until Transformation (Key Rule!)
```
xnf(φ₁ U φ₂) = (xnf(φ₂) ∧ ♢true) ∨ (xnf(φ₁) ∧ ◦(φ₁ U φ₂))
                                      ^^^^^^^^^^
                    KEEP ORIGINAL - DO NOT RECURSE!
```

**Explanation**:
- `xnf(φ₂) ∧ ♢true`: φ₂ holds eventually (and we're not at End)
- `xnf(φ₁) ∧ ◦(φ₁ U φ₂)`: φ₁ holds now, and φ₁ U φ₂ continues in Tail
- **Critical**: The inner `φ₁ U φ₂` is NOT recursively transformed!
- This is handled by rmnext progression during DFA construction

### Release Transformation (Dual Rule)
```
xnf(φ₁ R φ₂) = (xnf(φ₂) ∨ □false) ∧ (xnf(φ₁) ∨ •(φ₁ R φ₂))
                                      ^^^^^^^^^^
                    KEEP ORIGINAL - DO NOT RECURSE!
```

**Explanation**:
- `xnf(φ₂) ∨ □false`: φ₂ holds always (or we're at End)
- `xnf(φ₁) ∨ •(φ₁ R φ₂)`: φ₁ holds eventually, or φ₁ R φ₂ continues in Tail
- **Critical**: The inner `φ₁ R φ₂` is NOT recursively transformed!

---

## Implementation

### OpType Extensions

```cpp
enum class OpType {
    True, False,
    Not, And, Or,
    Next, WeakNext,    // ◦ (Strong Next), • (Weak Next)
    Until, Release,
    End,               // End marker for finite traces
    Literal            // Variable reference
};
```

### Conversion Rules for Our Design

**Weak Next (WX) Handling**:
```cpp
// During parsing: WX is directly converted to X
// User input: WX(φ)
// After parsing: X(φ)

// Rationale: We use End marker to handle finite traces
// Instead of having separate Weak Next operator
```

**Eventually/Always Conversion**:
```cpp
// ♢true → ¬End
// □false → End

// During XNF transformation:
Formula* create_eventually_true(FormulaPool& pool) {
    return pool.create_not(pool.create_end());
}

Formula* create_always_false(FormulaPool& pool) {
    return pool.create_end();
}
```

### XNF Transformation Algorithm

```cpp
// Main XNF transformation function
Formula* Formula::xnf_with_tail(FormulaPool& pool) const {
    switch (op_) {
        // Base cases: already in XNF
        case OpType::True:
        case OpType::False:
        case OpType::Literal:
        case OpType::Not:
        case OpType::End:
            return const_cast<Formula*>(this);

        case OpType::Next:
            // X(φ): recurse on child
            return pool.create_next(left_->xnf_with_tail(pool));

        // Boolean connectives: distribute
        case OpType::And:
            return pool.create_and(
                left_->xnf_with_tail(pool),
                right_->xnf_with_tail(pool)
            );

        case OpType::Or:
            return pool.create_or(
                left_->xnf_with_tail(pool),
                right_->xnf_with_tail(pool)
            );

        // Until: KEY TRANSFORMATION
        case OpType::Until: {
            Formula* left_xnf = left_->xnf_with_tail(pool);   // xnf(φ₁)
            Formula* right_xnf = right_->xnf_with_tail(pool);  // xnf(φ₂)

            // ♢true = ¬End
            Formula* eventually_true = pool.create_not(pool.create_end());

            // xnf(φ₂) ∧ ♢true
            Formula* right_part = pool.create_and(right_xnf, eventually_true);

            // xnf(φ₁) ∧ ◦(φ₁ U φ₂)
            // NOTE: Keep original Until formula, do NOT recurse!
            Formula* next_until = pool.create_next(
                const_cast<Formula*>(this)  // Original φ₁ U φ₂
            );
            Formula* left_part = pool.create_and(left_xnf, next_until);

            // (xnf(φ₂) ∧ ♢true) ∨ (xnf(φ₁) ∧ ◦(φ₁ U φ₂))
            return pool.create_or(right_part, left_part);
        }

        // Release: DUAL TRANSFORMATION
        case OpType::Release: {
            Formula* left_xnf = left_->xnf_with_tail(pool);   // xnf(φ₁)
            Formula* right_xnf = right_->xnf_with_tail(pool);  // xnf(φ₂)

            // □false = End
            Formula* always_false = pool.create_end();

            // xnf(φ₂) ∨ □false
            Formula* right_part = pool.create_or(right_xnf, always_false);

            // xnf(φ₁) ∨ •(φ₁ R φ₂)
            // NOTE: Keep original Release formula, do NOT recurse!
            // Note: • (Weak Next) is converted to X during parsing
            // So we use X here (will be handled correctly by rmnext)
            Formula* next_release = pool.create_next(
                const_cast<Formula*>(this)  // Original φ₁ R φ₂
            );
            Formula* left_part = pool.create_or(left_xnf, next_release);

            // (xnf(φ₂) ∨ □false) ∧ (xnf(φ₁) ∨ ◦(φ₁ R φ₂))
            return pool.create_and(right_part, left_part);
        }

        case OpType::WeakNext:
            // WX should have been converted to X during parsing
            // If we reach here, it's an error
            throw std::runtime_error("WeakNext should not appear in XNF");
    }
}
```

### Key Implementation Notes

1. **No Recursion on U/R**: The critical point is that we do NOT recursively call `xnf_with_tail()` on the `φ₁ U φ₂` inside the `X(φ₁ U φ₂)`.

2. **Immutable Input**: We use `const_cast<Formula*>(this)` to pass the original Until/Release formula. This is safe because formulas are immutable.

3. **Weak Next Conversion**: WX is converted to X during parsing, so we don't need to handle it in XNF.

4. **End Marker**: Created as singleton by `FormulaPool::create_end()`.

---

## Examples

### Example 1: Simple Until

**Input**:
```
φ = a U b
```

**XNF Transformation**:
```
xnf(a U b) = (b ∧ ♢true) ∨ (a ∧ X(a U b))
          = (b ∧ ¬End) ∨ (a ∧ X(a U b))
```

**Explanation**:
- Either `b` holds now (and we're not at End)
- Or `a` holds now, and `a U b` continues in Tail

**Result**:
- `pa(result) = {b, End, a, X(a U b)}`
- Only one temporal formula: `X(a U b)` (which is Next-form)

### Example 2: Nested Until

**Input**:
```
φ = (a ∧ X(b)) U c
```

**XNF Transformation**:
```
xnf(φ) = xnf((a ∧ X(b)) U c)
       = (xnf(c) ∧ ♢true) ∨ (xnf(a ∧ X(b)) ∧ X((a ∧ X(b)) U c))
       = (c ∧ ¬End) ∨ ((xnf(a) ∧ xnf(X(b))) ∧ X((a ∧ X(b)) U c))
       = (c ∧ ¬End) ∨ ((a ∧ X(xnf(b))) ∧ X((a ∧ X(b)) U c))
       = (c ∧ ¬End) ∨ ((a ∧ X(b)) ∧ X((a ∧ X(b)) U c))
```

**Result**:
- `pa(result) = {c, End, a, X(b), X((a ∧ X(b)) U c)}`
- All temporal formulas are Next-forms

### Example 3: User's Example

**Input**:
```
φ = (¬Tail ∧ a) U b
```

**Note**: `¬Tail` = `¬End` (Tail = End in our context)

**XNF Transformation**:
```
xnf((¬Tail ∧ a) U b)
 = (xnf(b) ∧ ♢true) ∨ (xnf(¬Tail ∧ a) ∧ X((¬Tail ∧ a) U b))
 = (b ∧ ¬End) ∨ ((xnf(¬Tail) ∧ xnf(a)) ∧ X((¬Tail ∧ a) U b))
 = (b ∧ ¬End) ∨ ((¬Tail ∧ a) ∧ X((¬Tail ∧ a) U b))
```

**Result**:
```
(b ∧ ¬End) ∨ ((¬End ∧ a) ∧ X((¬End ∧ a) U b))
```

**Verification**:
- `pa(result) = {b, End, a, X((¬End ∧ a) U b)}`
- Only Next-formulas in `pa(result)` ✓

### Example 4: Release

**Input**:
```
φ = a R b
```

**XNF Transformation**:
```
xnf(a R b) = (xnf(b) ∨ □false) ∧ (xnf(a) ∨ X(a R b))
          = (b ∨ End) ∧ (a ∨ X(a R b))
```

**Explanation**:
- Either `b` holds now, or we're at End
- And either `a` holds eventually, or `a R b` continues in Tail

### Example 5: Complex Formula

**Input**:
```
φ = (a U b) ∧ X(c)
```

**XNF Transformation**:
```
xnf(φ) = xnf(a U b) ∧ xnf(X(c))
       = ((b ∧ ¬End) ∨ (a ∧ X(a U b))) ∧ X(xnf(c))
       = ((b ∧ ¬End) ∨ (a ∧ X(a U b))) ∧ X(c)
```

**Result**:
- `pa(result) = {b, End, a, X(a U b), X(c)}`
- All temporal formulas are Next-forms ✓

---

## Integration with NNF

### Transformation Pipeline Order

```
Parse → NNF → XNF → rmnext
```

**Why NNF before XNF?**
1. NNF pushes all negations inward
2. XNF rules assume input is in NNF
3. This simplifies XNF transformation (no need to handle negations)

**Example**:
```
Input: !(a U b)

Step 1: NNF
  !(a U b) → (!a) R (!b)  (Until/Release duality)

Step 2: XNF
  xnf((!a) R (!b))
  = ((!b) ∨ End) ∧ ((!a) ∨ X((!a) R (!b)))

Output: ((!b) ∨ End) ∧ ((!a) ∨ X((!a) R (!b)))
```

### WX Handling

**Design Decision**: Convert WX to X during parsing

**Input Examples**:
```
User input: WX(a)
After parsing: X(a)
After NNF: X(a)
After XNF: X(a)

User input: !WX(a)
After parsing: !X(a)
After NNF: X(!a) | End
After XNF: (X(!a) | End)  # Already has End marker
```

**Rationale**:
- Simpler internal representation (no WeakNext operator needed)
- End marker handles finite trace semantics
- Compatible with rmnext progression

### Finite Trace Handling

**End Marker Role**:
1. **NNF**: Adds End when negating Next
   ```
   !X(φ) → X(!φ) | End
   ```

2. **XNF**: Uses End for ♢true/□false
   ```
   ♢true → ¬End
   □false → End
   ```

3. **rmnext**: Progression treats End specially
   ```
   End rmnext edge → False
   ```

---

## Complexity Analysis

**Time Complexity**: O(n) where n = formula size

**Justification**:
- Each subformula is transformed exactly once
- No recursion on Until/Release (only one-level expansion)
- Hash consing ensures structural sharing

**Space Complexity**: O(n) for the resulting formula

**Comparison with NNF**:
- NNF: O(n) - single pass pushing negations inward
- XNF: O(n) - single pass expanding U/R once
- Combined: O(n) - two linear passes

---

## Verification

### Correctness Criteria

XNF transformation is correct if:

1. **Preserves Semantics**: `xnf(φ)` is logically equivalent to `φ`
   - Verified by testing on sample formulas
   - Compare with original implementation

2. **XNF Property**: `pa(xnf(φ))` contains only literals, X, WX
   - Check: No Until/Release in primitive subformulas
   - Verify: All temporal formulas are Next-forms

3. **Finite Trace Correctness**: Handles End marker correctly
   - U-formulas do not accept empty traces (use ♢true)
   - R-formulas accept empty traces (use □false)

### Test Cases

```cpp
// Test 1: Simple Until
Formula* f1 = pool.create_until(a, b);
Formula* xnf1 = f1->xnf_with_tail(pool);
// Expected: (b ∧ ¬End) ∨ (a ∧ X(a U b))

// Test 2: Nested Until
Formula* inner = pool.create_next(b);
Formula* left = pool.create_and(a, inner);
Formula* f2 = pool.create_until(left, c);
Formula* xnf2 = f2->xnf_with_tail(pool);
// Expected: (c ∧ ¬End) ∨ ((a ∧ X(b)) ∧ X((a ∧ X(b)) U c))

// Test 3: Release
Formula* f3 = pool.create_release(a, b);
Formula* xnf3 = f3->xnf_with_tail(pool);
// Expected: (b ∨ End) ∧ (a ∨ X(a R b))

// Test 4: Combination
Formula* f4 = pool.create_and(
    pool.create_until(a, b),
    pool.create_next(c)
);
Formula* xnf4 = f4->xnf_with_tail(pool);
// Expected: ((b ∧ ¬End) ∨ (a ∧ X(a U b))) ∧ X(c)
```

---

## Summary

### Key Points

1. **XNF Definition**: `pa(φ)` only contains literals, X, WX (no U/R)
2. **Until Rule**: `xnf(φ₁ U φ₂) = (xnf(φ₂) ∧ ♢true) ∨ (xnf(φ₁) ∧ ◦(φ₁ U φ₂))`
3. **Release Rule**: `xnf(φ₁ R φ₂) = (xnf(φ₂) ∨ □false) ∧ (xnf(φ₁) ∨ •(φ₁ R φ₂))`
4. **No Recursion**: U/R inside Next is NOT recursively expanded
5. **End Marker**: Handles finite traces (♢true = ¬End, □false = End)
6. **Complexity**: O(n) linear time

### Design Decisions

- ✅ Use End marker for finite traces
- ✅ Convert WX to X during parsing
- ✅ One-level expansion of U/R (no recursion)
- ✅ ♢true → ¬End, □false → End
- ✅ Immutable formulas with hash consing

### Next Steps

1. Implement `xnf_with_tail()` in Formula class
2. Add End marker support to FormulaPool
3. Write comprehensive test cases
4. Verify with original implementation
5. Document rmnext progression (next step)

---

## References

- **Li et al. 2019**: "LTLf Synthesis as and via" - XNF transformation definition
- **De Giacomo & Vardi 2013**: LTLf semantics over finite traces
- **Original Implementation**: `aalta_formula.cpp::xnf_with_tail()`

---

**Status**: ✅ Design Complete, Implementation Pending
