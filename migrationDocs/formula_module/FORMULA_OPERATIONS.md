# Formula Operations: rmnext and aalta_formula

## Overview

This document describes the formula manipulation operations used in the target configuration:
- **COMB_FULL=1**: Full combination mode
- **USE_MINIMIZE=0**: No DFA minimization
- **comp_idx=1**: Incremental composition

It connects the **theoretical foundations** of LTLf (Linear Temporal Logic on Finite Traces) with the **practical implementation** in the Cosy synthesizer.

---

## Theoretical Background: LTLf Semantics

### Finite Traces vs Infinite Traces

Traditional LTL operates on **infinite traces** (lasso-shaped loops), while **LTLf** operates on **finite traces**. This fundamental difference affects the semantics of temporal operators, especially at the **end of trace**.

### Strong Next (X) vs Weak Next (W/N)

| Operator | Symbol | Semantics at End of Trace | Purpose |
|----------|--------|--------------------------|---------|
| Strong Next | X | **False** (requires next state) | Must have continuation |
| Weak Next | W / N | **True** (allows termination) | May terminate |

**Key Insight**: This distinction is crucial for LTLf synthesis, where traces can terminate legitimately.

### Tail Marker

To handle trace termination, LTLf introduces a special marker **Tail**:
- `Tail = true` indicates the current position is the **last state** of the trace
- All formulas in TNF (Tail Normal Form) include `F(Tail)` to enforce termination

---

## Core Data Structure: aalta_formula

### Formula Operators

```cpp
enum opkind {
    True,        // ⊤ (always true)
    False,       // ⊥ (always false)
    Literal,     // Atomic proposition (positive or negative)
    Not,         // ¬ (negation)
    And,         // ∧ (conjunction)
    Or,          // ∨ (disjunction)
    Next,        // X (Strong Next - requires next state)
    WNext,       // W (Weak Next - allows termination)
    Until,       // U (until)
    Release,     // R (release - dual of Until)
    Undefined
};
```

**Connection to Theory**:
- `Next` implements **Strong Next (X)** from LTLf theory
- `WNext` implements **Weak Next (W/N)** from LTLf theory
- The distinction between these operators is fundamental to correct LTLf semantics

### Formula Tree Structure

```cpp
class aalta_formula {
    aalta_formula *_left;   // Left sub-formula
    aalta_formula *_right;  // Right sub-formula
    int _op;                // Operator type
    aalta_formula *_simp;   // Cached simplified version
    aalta_formula *_unique; // Canonical pointer (structural equality)
};
```

**Design Patterns**:
- **Memoization**: `_simp` caches simplification results
- **Canonicalization**: `_unique` enables structural equality via pointer comparison
- **Lazy Evaluation**: Transformations computed on-demand and cached

### Key Methods

| Method | Purpose | Used in Target Config | Theory Connection |
|--------|---------|----------------------|-------------------|
| `nnf()` | Convert to Negation Normal Form | ✅ Yes | Pushes ¬ to atoms |
| `simplify()` | Apply logical equivalences | ✅ Yes | Normalization |
| `unique()` | Get canonical unique pointer | ✅ Yes | Structural hashing |
| `xnf_withTail()` | Convert to neXt Normal Form + Tail | ✅ Yes | Eliminates U/R |
| `l_af()` | Get left sub-formula | ✅ Yes | Tree navigation |
| `r_af()` | Get right sub-formula | ✅ Yes | Tree navigation |
| `oper()` | Get operator type | ✅ Yes | Pattern matching |

---

## Formula Transformations

### The Transformation Pipeline

```
Input Formula (LTLf string)
         ↓
    parse()
         ↓
aalta_formula tree (AST)
         ↓
    nnf()           ────→  Negation Normal Form
         ↓                    (¬ only before atoms)
    simplify()      ────→  Simplified NNF
         ↓                    (Flattened, normalized)
 xnf_withTail()    ────→  XNF with Tail handling
         ↓                    (U/R eliminated, Next expanded)
    rmnext()        ────→  State transition
         ↓                    (Formula progression)
```

---

### 1. NNF (Negation Normal Form)

**Theoretical Goal**: Push all negation operators to atomic propositions only.

**Transformation Rules**:
```
¬¬φ         →  φ                 (Double negation)
¬(φ ∧ ψ)    →  ¬φ ∨ ¬ψ           (De Morgan's)
¬(φ ∨ ψ)    →  ¬φ ∧ ¬ψ           (De Morgan's)
¬X φ        →  X ¬φ              (Next distributes)
¬(φ U ψ)    →  ¬φ R ¬ψ           (Dual operator)
¬(φ R ψ)    →  ¬φ U ¬ψ           (Dual operator)
```

**Implementation**:
```cpp
aalta_formula *aalta_formula::nnf() {
    // Recursive descent with memoization
    // Returns formula where ¬ only appears before literals
}
```

**Example**:
```
Original:  ¬(G (p ∧ q))
           ↓ (push negation inward)
NNF:       F (¬p ∨ ¬q)
           (G becomes F, ∧ becomes ∨, ¬ distributes)
```

---

### 2. Simplification

**Theoretical Goal**: Apply logical equivalences to normalize formula structure.

**Key Simplification Rules**:
```
// Identity elements
φ ∧ ⊤  →  φ          φ ∨ ⊥  →  φ

// Domination
φ ∧ ⊥  →  ⊥          φ ∨ ⊤  →  ⊤

// Idempotent
φ ∧ φ  →  φ          φ ∨ φ  →  φ

// Complement
φ ∧ ¬φ  →  ⊥          φ ∨ ¬φ  →  ⊤

// Associativity (flatten to chains)
(φ ∧ ψ) ∧ χ  →  φ ∧ ψ ∧ χ
(φ ∨ ψ) ∨ χ  →  φ ∨ ψ ∨ χ
```

**Implementation**: See `docs/SIMPLIFY_IMPLEMENTATION.md` for detailed analysis.

**Purpose in Pipeline**:
- Reduces formula size
- Canonicalizes structure (enables structural hashing)
- Prepares formula for XNF transformation

---

### 3. XNF (neXt Normal Form)

**Theoretical Goal**: Eliminate Until (U) and Release (R) operators by recursive expansion, leaving only Next operators.

**XNF Definition** (from theory):
> A formula is in XNF if its atomic set PA(φ) contains no sub-formulas of the form (φ₁ U φ₂) or (φ₁ R φ₂).

**Transformation Rules**:
```
// Until expansion
φ U ψ  ≡  ψ ∨ (φ ∧ X(φ U ψ))

// Release expansion (dual)
φ R ψ  ≡  ψ ∧ (φ ∨ X(φ R ψ))
```

**Why XNF?**
- Separates "current state constraints" from "next state obligations"
- Enables SAT/BDD-based state computation
- Critical for on-the-fly automaton construction

**XNF with Tail** (implementation variant):
```
X φ  →  φ ∧ ¬TAIL

This handles finite trace semantics:
- If we're at the end (TAIL=true), X φ fails
- Otherwise, X φ becomes φ for the next state
```

**Implementation**:
```cpp
aalta_formula *aalta_formula::xnf_withTail() {
    // Recursively eliminate U and R
    // Add ¬TAIL for each X operator
    // Cache results in static hash_map
}
```

**Example**:
```
Original:  F (p ∧ q)
           ↓ (expand F = True U ...)
NNF:       (p ∧ q) ∨ X(F(p ∧ q))
           ↓ (XNF with Tail)
XNF:       (p ∧ q) ∨ (X(F(p ∧ q)) ∧ ¬TAIL)
```

---

## Core Operation: rmnext (Formula Progression)

### Purpose and Theory

`rmnext(predecessor, edge)` computes the **next state formula** after applying a transition.

**Theoretical Connection**: This is the **FP (Formula Progression)** function from LTLf semantics:

```
ξ, i ⊨ φ    if and only if    ξ, i+1 ⊨ FP(φ, ξ(i))
```

Where:
- ξ is a trace (finite sequence of states)
- i is the current position
- FP (rmnext) computes the formula for position i+1

### Function Signature

```cpp
aalta_formula *rmnext(
    aalta_formula *predecessor,           // Current state formula (MUST be in XNF)
    aalta_formula *edge,                  // Transition (assignment to variables)
    const std::unordered_set<int> &all_vars_set  // All variables in the system
);
```

### Algorithm by Operator Type

#### For Literals (Atomic Propositions)

**Theory**: A literal evaluates to ⊤ or ⊥ based on whether it's satisfied by the current assignment.

```
Input:  p  (positive literal)
Edge:   {p, q, r}  (assignment where p=true, q=true, r=true)
Result: ⊤  (p is in the assignment)

Input:  ¬p  (negative literal)
Edge:   {p, q, r}
Result: ⊥  (p is in the assignment, so ¬p is false)
```

**Implementation**:
```cpp
case Literal:
    if (edge_contains_literal(predecessor, edge))
        return TRUE();   // ⊤
    else
        return FALSE();  // ⊥
```

#### For Boolean Operators

**Theory**: Progression distributes over boolean operators.

```
AND: φ ∧ ψ
rmnext(φ ∧ ψ, edge) = rmnext(φ, edge) ∧ rmnext(ψ, edge)

OR: φ ∨ ψ
rmnext(φ ∨ ψ, edge) = rmnext(φ, edge) ∨ rmnext(ψ, edge)
```

**Short-circuit Optimizations**:
```
// AND short-circuits
⊥ ∧ ψ  →  ⊥        (if left is ⊥, result is ⊥)
⊤ ∧ ψ  →  ψ        (if left is ⊤, result is right)

// OR short-circuits
⊤ ∨ ψ  →  ⊤        (if left is ⊤, result is ⊤)
⊥ ∨ ψ  →  ψ        (if left is ⊥, result is right)
```

#### For Temporal Operators (The Critical Part)

**Strong Next (X φ)**:
```
rmnext(X φ, edge) = φ ∧ ¬TAIL

Theory Explanation:
1. We consume one step (progress from position i to i+1)
2. The X operator is "used up"
3. φ becomes the formula for the next position
4. We add ¬TAIL because we're no longer at the end (we just moved!)
```

**Weak Next (W φ)**:
```
rmnext(W φ, edge) = φ ∨ TAIL

Theory Explanation:
1. Weak Next allows two possibilities:
   a. φ holds in the next state, OR
   b. We're at the end of the trace (no next state required)
2. This matches LTLf semantics where W φ is true at the end
```

**Why TAIL matters**:
- **Without TAIL**: We couldn't distinguish "must continue" from "may terminate"
- **With TAIL**: Strong Next (X) forces continuation, Weak Next (W) allows termination

#### For Until/Release (Should Not Appear)

```
These operators should NOT appear if predecessor is in XNF format.
If they appear, it indicates a bug in the XNF transformation.
```

**Reason**: XNF is specifically designed to eliminate U and R operators.

### Detailed Example

**Scenario**: Computing successor state from current state

```
Current State Formula: X(p ∨ q)
                      ↑
                      Strong Next: must have continuation

Edge (Transition):
  Inputs:  {env=true}
  Outputs: {p=true, r=true}

Step 1: Verify formula is in XNF
  X(p ∨ q) is already in XNF form

Step 2: Apply rmnext
  rmnext(X(p ∨ q), {env, p, r})
  = (p ∨ q) ∧ ¬TAIL     [by Strong Next rule]

Step 3: Evaluate boolean part against edge
  rmnext(p ∨ q, {env, p, r})
  = rmnext(p, {env, p, r}) ∨ rmnext(q, {env, p, r})
  = ⊤ ∨ ⊥                [p is in edge, q is not]
  = ⊤

Step 4: Final result
  ⊤ ∧ ¬TAIL = ¬TAIL

Interpretation:
  The next state must satisfy "not at end of trace"
  This means the trace MUST continue (enforced by Strong Next)
```

---

## Key Utility Functions

### getAndSubAfs

```cpp
std::vector<aalta_formula *> getAndSubAfs(aalta_formula *af);
```

**Purpose**: Split AND formula into sub-formulas (conjuncts)

**Example**:
```
Input:  (F p) ∧ (F q) ∧ (G r)
Output: [F p, F q, G r]

Used in: Incremental composition loop to process each conjunct separately
```

**Theory Connection**: Decomposes a conjunction into independent sub-problems for incremental synthesis.

---

### isAccByEmptyTrace

```cpp
bool isAccByEmptyTrace(aalta_formula *af);
```

**Purpose**: Check if formula is satisfied by **empty trace** (trace of length 0)

**Returns true for**:
- ⊤ (True)
- Formulas that don't require any steps to satisfy

**Theory Connection**: In LTLf, some formulas are vacuously true on empty traces:
- ⊤ is trivially true
- F φ is **false** on empty trace (requires φ to occur)
- G φ is **true** on empty trace (vacuously holds)

**Used in**: Empty acceptance check during state visit to determine if a state can be accepting immediately.

---

### FormulaProgression

```cpp
aalta_formula *FormulaProgression(
    aalta_formula *predecessor,
    aalta_formula *edge,
    const std::unordered_set<int> &all_vars_set
);
```

**Purpose**: Alias for `rmnext` - computes next state formula

**Theory**: This is the FP function formalized in LTLf literature.

---

## Formula Formats and Transformation Chain

### Input Format
```
LTLf formula as string: "G F p ∧ F q"
                         ↓
Parsed to aalta_formula tree with operators
```

### Internal Processing Pipeline
```
1. Parse → aalta_formula tree
              ↓
2. nnf()  → Negation Normal Form
              (¬ only before atoms)
              ↓
3. simplify() → Simplified NNF
              (flattened AND/OR chains)
              ↓
4. xnf_withTail() → XNF with TAIL
              (U/R eliminated, X handles Tail)
              ↓
5. rmnext() → State progression
              (compute successor formulas)
```

### Complete Transformation Example

```
Original:  ¬(G (p ∧ q))
           ↓ [Parse]
AST:       ¬(G(p ∧ q))
           ↓ [NNF: push ¬, G→F, ∧→∨]
NNF:       F (¬p ∨ ¬q)
           ↓ [Simplify: no change]
Simplified: F (¬p ∨ ¬q)
           ↓ [XNF: expand F = True U (...)]
XNF:       (¬p ∨ ¬q) ∨ (X(F(¬p ∨ ¬q)) ∧ ¬TAIL)
           ↑           ↑
           Current     Next obligation
           state       with Tail handling
```

**Interpretation**:
- Current state must satisfy `(¬p ∨ ¬q)`, OR
- Move to next state (where formula becomes `F(¬p ∨ ¬q)`), AND
- We're not at end of trace (`¬TAIL`)

---

## Usage in Target Configuration

### In WholeDFA_TarjanStrategy (Individual Synthesis)

**File**: `lib/include/ltlfsyn/syn_tarjan.cpp:278`

```cpp
aalta_formula *next_state_af = rmnext(
    cur_state_af_xnfWithTail,  // Current state formula (in XNF format)
    edge_af,                   // Transition (assignment to inputs/outputs)
    this->fibMgr_.getAllVarIds() // All variables in the system
);

// next_state_af becomes the formula for the successor state
// This is used to build the DFA transition system
```

**Theory Connection**: This call implements the **LTLf Transition System** construction described in theory:
- States are formulas (sub-formulas of the original)
- Transitions are computed via rmnext
- The resulting DFA is the **automaton for the LTLf formula**

---

## Important Implementation Details

### 1. Formula Caching (Memoization)

Both `xnf()` and `xnf_withTail()` use memoization:
```cpp
static hash_map<aalta_formula *, aalta_formula *> f_to_xnf;
static hash_map<aalta_formula *, aalta_formula *> f_to_xnfWithTail;
```

**Benefits**:
- Avoids recomputing the same transformation
- Crucial for performance (transformations are recursive/expensive)
- Enables structural equality via `unique()` pointers

---

### 2. Unique Pointers (Canonicalization)

`unique()` returns a canonical pointer for structurally identical formulas:
```cpp
aalta_formula *f1 = new aalta_formula(...);
aalta_formula *f2 = new aalta_formula(...);
// If f1 and f2 are structurally identical:
assert(f1->unique() == f2->unique());
```

**Theory Connection**: Implements **hash consing** - a technique for:
- Fast equality checking (pointer comparison vs. tree comparison)
- Automatic deduplication
- Memory efficiency

---

### 3. XNF Format Requirement

`rmnext` **requires** the predecessor to be in **XNF format**:
- ✅ No Until/Release at the top level
- ✅ Temporal operators only appear within specific transformations
- ✅ Next operators handle Tail semantics

**Why this requirement?**
- rmnext's algorithm assumes specific structure
- Until/Release would require more complex progression
- XNF ensures efficient linear-time processing

**Enforcement**: Always call `xnf_withTail()` before `rmnext()`:
```cpp
auto xnf_form = formula->xnf_withTail();
auto next = rmnext(xnf_form, edge, vars);
```

---

### 4. TAIL Constant (Finite Trace Semantics)

`TAIL` represents "end of trace" in LTLf semantics:

```
TAIL = true    ← Current position is last state
TAIL = false   ← Current position has successors

NOT_TAIL = ¬TAIL
```

**Usage in rmnext**:
```
X φ  →  rmnext(X φ) = φ ∧ ¬TAIL
W φ  →  rmnext(W φ) = φ ∨ TAIL
```

**Theory Connection**: This implements the **Tail Normal Form (TNF)** from LTLf theory:
- Every formula in TNF includes `F(Tail)` to enforce termination
- In XNF, Tail is handled explicitly in Next operators

---

## File Locations

| File | Purpose |
|------|---------|
| `lib/deps/formula/aalta_formula.h` | Formula class definition |
| `lib/deps/formula/aalta_formula.cpp` | Formula method implementations |
| `lib/deps/formula/af_utils.h` | Formula utility functions |
| `lib/deps/formula/af_utils.cpp` | Utility function implementations (rmnext) |
| `lib/include/ltlfsyn/syn_tarjan.cpp` | Usage in synthesis (line 278) |
| `docs/LTLF_BASIC.md` | Theoretical foundations |
| `docs/SIMPLIFY_IMPLEMENTATION.md` | Simplify series analysis |

---

## Complexity Analysis

### rmnext Complexity

- **Time**: O(|formula|) where |formula| is formula size
- **Space**: O(|formula|) for the result formula
- **With caching**: Amortized O(1) for repeated calls with same formula

**Why linear?**
- Single traversal of formula tree
- Each operator handled in constant time
- No backtracking or search required

### xnf Transformation

- **Time**: O(|formula|) for each unique formula
- **Space**: O(total unique formulas) due to caching
- **Can be exponential** in worst case (Until expansion creates recursive sub-formulas)

**Why exponential?**
```
φ U ψ  →  ψ ∨ (φ ∧ X(φ U ψ))
               ↑
           Recursive occurrence
```
But caching makes it practical for most formulas.

---

## Summary

### Connection Between Theory and Implementation

| Theoretical Concept | Implementation | Purpose |
|---------------------|----------------|---------|
| **LTLf Semantics** | `rmnext()` function | Formula progression on finite traces |
| **Strong Next (X)** | `Next` operator | Forces continuation |
| **Weak Next (W/N)** | `WNext` operator | Allows termination |
| **Tail Marker** | `TAIL` constant | Marks end of trace |
| **NNF** | `nnf()` method | Push ¬ to atoms |
| **XNF** | `xnf_withTail()` method | Eliminate U/R, prepare for progression |
| **Transition System** | DFA construction | Build automaton from formula |
| **Satisfiability** | `isAccByEmptyTrace()` | Check empty trace acceptance |

### Key Operations in Target Config

For **COMB_FULL=1, USE_MINIMIZE=0, comp_idx=1**:

1. **Parsing**: LTLf string → aalta_formula tree
2. **Normalization**:
   - `nnf()` → Negation Normal Form
   - `simplify()` → Canonical structure
   - `xnf_withTail()` → Eliminate U/R, add Tail handling
3. **Decomposition**: `getAndSubAfs()` to split AND formulas
4. **Progression**: `rmnext()` to compute successor state formulas
5. **Checking**: `isAccByEmptyTrace()` for empty acceptance

These operations implement the **LTLf to DFA construction** that forms the foundation of reactive synthesis in Cosy.

---

## Further Reading

- **Theoretical Foundations**: See `docs/LTLF_BASIC.md` for LTLf semantics and normal forms
- **Implementation Details**: See `docs/SIMPLIFY_IMPLEMENTATION.md` for simplify series analysis
- **System Architecture**: See `docs/COMB_FULL_1_COMP_IDX_1_ANALYSIS.md` for complete system flow
- **Edge Constraints**: See `docs/EDGE_CONSTRAINT_SPEC.md` for transition encoding
