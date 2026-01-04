# NNF Transformation Algorithm

**Date**: 2025-01-01
**Status**: Design Complete, Ready for Implementation

---

## Table of Contents

1. [Introduction](#introduction)
2. [Theoretical Foundation](#theoretical-foundation)
3. [Key Concepts](#key-concepts)
4. [NNF Transformation Rules](#nnf-transformation-rules)
5. [Implementation](#implementation)
6. [Examples](#examples)
7. [Finite Trace Handling](#finite-trace-handling)

---

## Introduction

**NNF (Negation Normal Form)**: A formula is in NNF if all negation operators appear only directly in front of atomic propositions (literals).

**Goal**:
- Push all negations inward using De Morgan's laws and duality rules
- Eliminate double negations
- Make formula structure more regular for subsequent transformations
- Prepare for XNF, simplify, and rmnext

**Transformation Pipeline**:
```
Parse → NNF → XNF → (rmnext during DFA construction)
```

---

## Theoretical Foundation

### Definition

**NNF Formula**: An LTLf formula φ is in Negation Normal Form if negation operators (¬) appear only in front of:
- Atomic propositions (a, b, c, ...)
- Negated atomic propositions (¬a, ¬b, ¬c, ...)

**In other words**: No negation appears in front of compound formulas (And, Or, temporal operators).

### Why NNF?

1. **Simplifies Subsequent Transformations**: XNF, simplify, and rmnext assume input is in NNF
2. **Regular Structure**: Easier to reason about and implement
3. **Enables Optimizations**: Many optimization rules work on NNF formulas
4. **Standard Practice**: NNF is a standard form in temporal logic

### LTLf vs Standard LTL

**Standard LTL** (infinite traces):
- Negation normal form: Push negations inward using duality
- `¬X(φ) ≡ X(¬φ)` works perfectly

**LTLf** (finite traces):
- Same NNF rules, BUT...
- Need special handling for `¬X(φ)` at the final position
- Solution: Use **End** marker

---

## Key Concepts

### End Marker

**End** (or **Last**): A special atomic proposition marking the final position in a finite trace.

**Semantics**:
```
π, i ⊨ End  iff  i is the last position of trace π
```

**Role in NNF**:
- Handles negation of Strong Next in finite traces
- `¬X(φ)` must consider: "either there's a next position where ¬φ holds, OR this is the end"

### Weak Next (WX)

**Definition**: WX(φ) means "φ holds in the next position, if there is one".

**Semantics**:
```
π, i ⊨ WX(φ)  iff  (i is last position) OR (π, i+1 ⊨ φ)
```

**Design Decision**: Convert WX to X during parsing
- User input: `WX(a)` → Parser → `X(a)`
- End marker handles finite trace semantics
- Simplifies internal representation (no separate WeakNext operator)

### Duality Rules

**Temporal operators come in dual pairs**:

| Operator | Dual | Transformation |
|----------|------|----------------|
| Until (U) | Release (R) | `¬(φ U ψ) ≡ (¬φ) R (¬ψ)` |
| Release (R) | Until (U) | `¬(φ R ψ) ≡ (¬φ) U (¬ψ)` |
| Finally (F) | Globally (G) | `¬F(φ) ≡ G(¬φ)` |
| Globally (G) | Finally (F) | `¬G(φ) ≡ F(¬φ)` |

These dualities allow us to push negations inward.

---

## NNF Transformation Rules

### Base Cases (Already in NNF)

```
True        → True
False       → False
Literal(a)  → Literal(a)      // a is atomic variable
Not(a)      → Not(a)           // a is atomic variable (already in NNF)
```

**Note**: `Not(a)` is already in NNF because negation is directly in front of a literal.

### Double Negation Elimination

```
Not(Not(φ)) → φ
```

**Example**:
```
Not(Not(a)) → a
Not(Not(¬a)) → ¬a
```

### De Morgan's Laws (Boolean Operators)

**And**:
```
Not(φ ∧ ψ) → (¬φ) ∨ (¬ψ)
```

**Or**:
```
Not(φ ∨ ψ) → (¬φ) ∧ (¬ψ)
```

**Examples**:
```
Not(a ∧ b) → ¬a ∨ ¬b
Not(a ∨ b) → ¬a ∧ ¬b
Not(a ∧ (b ∨ c)) → ¬a ∨ Not(b ∨ c) → ¬a ∨ (¬b ∧ ¬c)
```

### Temporal Operators - Duality Rules

**Finally (F) and Globally (G)**:
```
Not(F(φ)) → G(¬φ)        // F = "eventually", G = "always"
Not(G(φ)) → F(¬φ)        // Dual operators
```

**Until (U) and Release (R)**:
```
Not(φ U ψ) → (¬φ) R (¬ψ)     // U and R are dual
Not(φ R ψ) → (¬φ) U (¬ψ)
```

**Strong Next (X) - LTLf Finite Trace Handling**:
```
Not(X(φ)) → X(¬φ) ∨ End      // End marker for finite traces!
```

**Critical**: This is the ONLY rule that differs from standard LTL!

### Weak Next (WX) - Optional Input Syntax

```
WX(φ) → X(φ)                 // Direct conversion during parsing
Not(WX(φ)) → X(¬φ)           // No End needed!
```

**Rationale**: WX is semantically `X(φ) ∨ End`, so:
- `WX(φ)` → `X(φ)` (End will be added during XNF if needed)
- `¬WX(φ)` → `¬(X(φ) ∨ End)` → `¬X(φ) ∧ ¬End` → `X(¬φ) ∧ ¬End` (but simplified to `X(¬φ)`)

---

## Implementation

### OpType Definitions

```cpp
enum class OpType {
    True, False,           // Constants
    Not, And, Or,           // Boolean operators
    Next, WeakNext,        // ◦ (Strong Next), • (Weak Next)
    Until, Release,        // Temporal operators
    End,                   // End marker for finite traces
    Literal                // Variable reference
};
```

### NNF Transformation Algorithm

```cpp
// Helper function for NNF transformation of negated formulas
Formula* to_nnf_not(FormulaPool& pool, Formula* f) {
    switch (f->op()) {
        case OpType::True:
            return pool.create_false();

        case OpType::False:
            return pool.create_true();

        case OpType::Literal:
            // Not(a) where a is atomic variable - already in NNF
            return pool.create_not(f);

        case OpType::Not:
            // Double negation: Not(Not(φ)) → φ
            return f->left()->nnf(pool);

        case OpType::And:
            // De Morgan: Not(φ ∧ ψ) → (¬φ) ∨ (¬ψ)
            return pool.create_or(
                to_nnf_not(pool, f->left()),
                to_nnf_not(pool, f->right())
            );

        case OpType::Or:
            // De Morgan: Not(φ ∨ ψ) → (¬φ) ∧ (¬ψ)
            return pool.create_and(
                to_nnf_not(pool, f->left()),
                to_nnf_not(pool, f->right())
            );

        case OpType::Next:
            // LTLf finite trace: Not(X(φ)) → X(¬φ) ∨ End
            return pool.create_or(
                pool.create_next(
                    to_nnf_not(pool, f->left())
                ),
                pool.create_end()  // End marker
            );

        case OpType::Until:
            // Duality: Not(φ U ψ) → (¬φ) R (¬ψ)
            return pool.create_release(
                to_nnf_not(pool, f->left()),
                to_nnf_not(pool, f->right())
            );

        case OpType::Release:
            // Duality: Not(φ R ψ) → (¬φ) U (¬ψ)
            return pool.create_until(
                to_nnf_not(pool, f->left()),
                to_nnf_not(pool, f->right())
            );

        case OpType::End:
            // Not(End) is already in NNF
            // End is like a literal
            return pool.create_not(f);
    }
}

// Main NNF transformation function
Formula* Formula::nnf(FormulaPool& pool) const {
    switch (op_) {
        // Base cases: already in NNF
        case OpType::True:
        case OpType::False:
        case OpType::Literal:
        case OpType::End:
            return const_cast<Formula*>(this);

        case OpType::Not:
            // Handle negation using helper
            return to_nnf_not(pool, left_);

        // Boolean operators: recurse on children
        case OpType::And:
        case OpType::Or:
            return pool.create(
                op_,
                left_->nnf(pool),
                right_->nnf(pool)
            );

        // Temporal operators: recurse on children
        case OpType::Until:
        case OpType::Release:
            return pool.create(
                op_,
                left_->nnf(pool),
                right_->nnf(pool)
            );

        case OpType::Next:
            // Structure-preserving: recurse on child
            return pool.create_next(left_->nnf(pool));

        case OpType::WeakNext:
            // WX should have been converted to X during parsing
            throw std::runtime_error("WeakNext should not appear in NNF");
    }
}
```

### End Marker Implementation

**FormulaPool Support**:
```cpp
class FormulaPool {
public:
    // Create End marker (singleton)
    Formula* create_end() {
        if (!end_marker_) {
            end_marker_ = create(OpType::End, nullptr, nullptr, -1);
        }
        return end_marker_;
    }

private:
    Formula* end_marker_ = nullptr;  // Singleton End marker
};
```

**Progression in rmnext**:
When applying rmnext to End:
```cpp
case OpType::End:
    // End marker: no next state, return False
    return pool.create_false();
```

---

## Examples

### Example 1: Simple De Morgan

**Input**:
```
!(a & b)
```

**Transformation Steps**:
```
Step 1: Apply to_nnf_not to And
Step 2: Not(a & b) → Not(a) ∨ Not(b)  (De Morgan)
Step 3: Not(a) and Not(b) are already in NNF
```

**Output**:
```
(¬a) ∨ (¬b)
```

**Verification**: No negation in front of compound formulas ✓

### Example 2: Double Negation

**Input**:
```
!!a
```

**Transformation**:
```
!!a → Not(Not(a)) → a
```

**Output**: `a`

### Example 3: Next Negation with End

**Input**:
```
!X(a)
```

**Transformation Steps**:
```
Step 1: Apply to_nnf_not to Next
Step 2: Not(X(a)) → X(Not(a)) ∨ End
Step 3: Not(a) is already in NNF
```

**Output**:
```
X(¬a) ∨ End
```

**Explanation**:
- Either there's a next position where `¬a` holds: `X(¬a)`
- OR this is the final position: `End`

### Example 4: Complex Formula with Until

**Input**:
```
!(a U X(b))
```

**Transformation Steps**:
```
Step 1: Apply to_nnf_not to Until
Step 2: Not(a U X(b)) → Not(a) R Not(X(b))
Step 3: Not(a) is already in NNF (¬a)
Step 4: Not(X(b)) → X(¬b) ∨ End
```

**Output**:
```
(¬a) R (X(¬b) ∨ End)
```

**Verification**:
- `¬a` is a literal ✓
- `X(¬b) ∨ End` has no negation in front of compound formulas ✓
- Only Release operator remains ✓

### Example 5: Finally/Until Duality

**Input**:
```
!F(a)
```

**Transformation**:
```
Step 1: F(a) = true U a (definition of Finally)
Step 2: Not(true U a) → Not(true) R Not(a)
Step 3: Not(true) = false
Step 4: Not(a) is already in NNF
```

**Output**:
```
false R (¬a)
```

**Alternative** (using direct F/G duality):
```
Not(F(a)) → G(Not(a)) → (false R Not(a))
```

Both produce the same result!

### Example 6: Nested Negations

**Input**:
```
!(a & !!(b & c))
```

**Transformation Steps**:
```
Step 1: !(a & !!(b & c)) → !(a & (b & c))  (eliminate !!)
Step 2: Not(a & (b & c)) → Not(a) ∨ Not(b & c)
Step 3: Not(a) ∨ (Not(b) ∨ Not(c))  (De Morgan on b & c)
```

**Output**:
```
¬a ∨ (¬b ∨ ¬c)
```

### Example 7: Weak Next Conversion

**Input (user syntax)**:
```
WX(a)
```

**After Parsing**:
```
X(a)  // WX converted to X
```

**After NNF**:
```
X(a)  // No change, already in NNF
```

**Input (user syntax)**:
```
!WX(a)
```

**After Parsing**:
```
!X(a)  // WX converted to X
```

**After NNF**:
```
X(!a) | End  // Apply Next negation rule
```

**Note**: The `End` is only added when negating Strong Next, not when using WX directly.

---

## Finite Trace Handling

### The Challenge

In LTLf (finite traces), `¬X(φ)` requires special handling:

**Standard LTL** (infinite traces):
```
¬X(φ) ≡ X(¬φ)
```
This works because there's always a next position.

**LTLf** (finite traces):
```
¬X(φ) needs to consider:
1. There's a next position where ¬φ holds: X(¬φ)
2. This IS the last position (no next exists): End
```

### The Solution: End Marker

**Transformation Rule**:
```
¬X(φ) ≡ X(¬φ) ∨ End
```

**Examples**:
```
¬X(a) → X(¬a) ∨ End
¬X(a ∧ b) → X(¬(a ∧ b)) ∨ End → X(¬a ∨ ¬b) ∨ End
¬X(X(a)) → X(¬X(a)) ∨ End → X(X(¬a) ∨ End) ∨ End
```

### Why This Works

**Case 1**: Current position is NOT the last
- `End` is False
- Formula simplifies to: `X(¬φ)`
- Correct: "¬φ holds in the next position"

**Case 2**: Current position IS the last
- `End` is True
- Formula simplifies to: `True`
- Correct: "¬X(φ) is vacuously true when there's no next position"

### Comparison with Weak Next

**Strong Next (X)**:
```
X(φ) at End → False  (must have next position)
¬X(φ) → X(¬φ) ∨ End
```

**Weak Next (WX)**:
```
WX(φ) at End → True  (vacuously true)
¬WX(φ) → X(¬φ)  (no End needed!)
```

This is why we convert WX to X during parsing and use End marker to handle the difference.

---

## Complexity Analysis

**Time Complexity**: O(n) where n = formula size

**Justification**:
- Each subformula is visited exactly once
- Each transformation rule is O(1)
- No backtracking or iteration needed

**Space Complexity**: O(n) for the resulting formula

**Comparison**:
- NNF: O(n) - single pass pushing negations inward
- XNF: O(n) - single pass expanding U/R once
- Combined: O(n) - two linear passes

---

## Verification

### Correctness Criteria

NNF transformation is correct if:

1. **Preserves Semantics**: `nnf(φ)` is logically equivalent to `φ`
2. **NNF Property**: No negation in front of compound formulas
3. **Finite Trace Correctness**: Handles End marker correctly

### Test Cases

```cpp
// Test 1: De Morgan
Formula* f1 = pool.create_and(a, b);
Formula* not_f1 = pool.create_not(f1);
Formula* nnf1 = not_f1->nnf(pool);
// Expected: Or(Not(a), Not(b))

// Test 2: Double negation
Formula* f2 = pool.create_not(pool.create_not(a));
Formula* nnf2 = f2->nnf(pool);
// Expected: a

// Test 3: Next negation
Formula* f3 = pool.create_next(a);
Formula* not_f3 = pool.create_not(f3);
Formula* nnf3 = not_f3->nnf(pool);
// Expected: Or(Next(Not(a)), End)

// Test 4: Until duality
Formula* f4 = pool.create_until(a, b);
Formula* not_f4 = pool.create_not(f4);
Formula* nnf4 = not_f4->nnf(pool);
// Expected: Release(Not(a), Not(b))

// Test 5: Nested formula
Formula* inner = pool.create_next(b);
Formula* left = pool.create_and(a, inner);
Formula* f5 = pool.create_until(left, c);
Formula* not_f5 = pool.create_not(f5);
Formula* nnf5 = not_f5->nnf(pool);
// Expected: Release(Not(a), Or(Next(Not(b)), End))
```

---

## Summary

### Key Points

1. **NNF Definition**: Negations only in front of literals
2. **De Morgan's Laws**: Push negations through And/Or
3. **Duality Rules**: U↔R, F↔G for pushing negations through temporal operators
4. **Double Negation**: `¬¬φ ≡ φ`
5. **End Marker**: Handles `¬X(φ)` in finite traces
6. **Weak Next**: Converted to X during parsing

### Design Decisions

- ✅ Use End marker for finite traces
- ✅ Convert WX to X during parsing
- ✅ Recursive transformation (bottom-up)
- ✅ Immutable formulas with hash consing
- ✅ O(n) linear time complexity

### Next Steps

1. Implement `nnf()` in Formula class
2. Implement `to_nnf_not()` helper function
3. Add End marker support to FormulaPool
4. Write comprehensive test cases
5. Verify with original implementation
6. Proceed to XNF transformation

---

## References

- **De Giacomo & Vardi 2013**: LTLf semantics over finite traces
- **Original Implementation**: `aalta_formula.cpp::nnf()`
- **Related**: [XNF_TRANSFORMATION.md](./XNF_TRANSFORMATION.md) - Next transformation

---

**Status**: ✅ Design Complete, Implementation Pending
