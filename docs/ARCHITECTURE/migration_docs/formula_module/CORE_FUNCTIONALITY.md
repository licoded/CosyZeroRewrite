# Core Functionality to Keep - Formula Module Redesign

## Overview

This document extracts the **essential functionality** from `aalta_formula.h/cpp` and `af_utils.h/cpp` that should be preserved in the redesign. Everything else can be removed or redesigned.

---

## Analysis Method

**What was analyzed:**
- 5,232 total lines across 4 files
- Actual usage in Cosy codebase (`lib/include/`, `app/src/`)
- Call graph analysis to find truly used functions

**Key finding:** Only ~20% of the code is actively used in the target configuration (COMB_FULL=1, USE_MINIMIZE=0, comp_idx=1).

---

## Core Functionality to Keep

### 1. Formula Data Structure (AST)

**Location**: `aalta_formula.h` core structure

**Required members**:
```cpp
class Formula {
    int _op;                      // Operator type (enum)
    Formula *_left;               // Left child (NULL for unary/leaf)
    Formula *_right;              // Right child (NULL for leaf)
    int _id;                      // Unique ID for hashing
};
```

**Operator types** (essential only):
```cpp
enum OpKind {
    True,        // ⊤
    False,       // ⊥
    Literal,     // Atomic propositions (ID >= 11)
    Not,         // ¬ (only before literals in NNF)
    And,         // ∧
    Or,          // ∨
    Next,        // X (Strong Next)
    WNext,       // W (Weak Next) - CRITICAL for LTLf
    Until,       // U (eliminated by XNF)
    Release      // R (eliminated by XNF)
};
```

**Why these are essential:**
- True/False/Literal/Not: Base values
- And/Or: Boolean logic
- Next/WNext: **Critical distinction** for LTLf finite trace semantics
- Until/Release: Needed for parsing, but eliminated by XNF transformation

**Can be removed:**
- Tag system (`_tag`, `tag_t`) - Only used for specific Lydia integration
- `_simp` cache field - Can be externalized
- `_unique` field - Can be externalized

---

### 2. Core Transformations

#### 2.1 Parsing

**Function**: `aalta_formula(const char* input, bool is_ltlf)`

**Purpose**: Parse LTLf string into AST

**Used in**:
- `lib/include/ltlfsyn/syn_tarjan.cpp`
- `lib/include/dfa_combine/dfs_product.cpp`

**Must keep**: Yes, this is the entry point

---

#### 2.2 NNF (Negation Normal Form)

**Function**: `aalta_formula *nnf()`

**Purpose**: Push negations to atomic propositions only

**Transformation rules**:
```
¬¬φ       →  φ
¬(φ ∧ ψ)  →  ¬φ ∨ ¬ψ
¬(φ ∨ ψ)  →  ¬φ ∧ ¬ψ
¬X φ      →  X ¬φ
¬(φ U ψ)  →  ¬φ R ¬ψ
¬(φ R ψ)  →  ¬φ U ¬ψ
```

**Used in**: Every synthesis path

**Must keep**: Yes, required before XNF

---

#### 2.3 XNF with Tail (Extended Normal Form)

**Function**: `aalta_formula *xnf_withTail(aalta_formula *phi)`

**Location**: `af_utils.cpp:163-247`

**Purpose**: Eliminate U/R operators, add TAIL handling for finite traces

**Key transformations**:
```cpp
// Until expansion with TAIL
φ U ψ  →  (ψ ∧ ¬TAIL) ∨ (φ ∧ X(φ U ψ))

// Release expansion with TAIL
φ R ψ  →  (ψ ∨ TAIL) ∧ (φ ∨ W(φ R ψ))

// Strong Next with TAIL
X φ    →  φ ∧ ¬TAIL  (in rmnext, not xnf)

// Weak Next with TAIL
W φ    →  φ ∨ TAIL   (in rmnext, not xnf)
```

**Used in**:
- `lib/include/ltlfsyn/syn_tarjan.cpp:277`

**Must keep**: **Critical** - This is the core of LTLf semantics

---

#### 2.4 Simplification

**Function**: `aalta_formula *simplify()`

**Purpose**: Apply logical equivalences to normalize formula

**Key rules** (simplified subset):
```
φ ∧ ⊤  →  φ          φ ∨ ⊥  →  φ
φ ∧ ⊥  →  ⊥          φ ∨ ⊤  →  ⊤
φ ∧ φ  →  φ          φ ∨ φ  →  φ
φ ∧ ¬φ  →  ⊥         φ ∨ ¬φ  →  ⊤
```

**Used in**:
- `af_utils.cpp:471` (inside `rmnext`)
- `lib/include/ltlfsat/solver.cpp`

**Must keep**: Yes, but can be significantly simplified

---

### 3. Formula Progression (rmnext)

**Function**: `aalta_formula *rmnext(predecessor, edge, all_vars_set)`

**Location**: `af_utils.cpp:462-473`

**Purpose**: Compute next state formula after applying a transition

**Signature**:
```cpp
aalta_formula *rmnext(
    aalta_formula *predecessor,           // Current state (in XNF format)
    aalta_formula *edge,                  // Transition assignment
    const std::unordered_set<int> &all_vars_set
);
```

**Algorithm by operator**:
```
// Literals
p:       edge ⊨ p  →  ⊤  |  edge ⊭ p  →  ⊥
¬p:      edge ⊨ p  →  ⊥  |  edge ⊭ p  →  ⊤

// Boolean (distributive)
φ ∧ ψ:   rmnext(φ) ∧ rmnext(ψ)
φ ∨ ψ:   rmnext(φ) ∨ rmnext(ψ)

// Temporal (TAIL-aware)
X φ:     φ ∧ ¬TAIL    ← Strong Next forces continuation
W φ:     φ ∨ TAIL     ← Weak Next allows termination
```

**Used in**:
- `lib/include/ltlfsyn/syn_tarjan.cpp:278`

**Must keep**: **Critical** - This is the core operation for state transition

---

### 4. And-Formula Decomposition

**Function**: `std::vector<aalta_formula *> getAndSubAfs(aalta_formula *af)`

**Location**: `af_utils.cpp:617-634`

**Purpose**: Split AND formula into conjuncts, sort by heuristics

**Algorithm**:
```
1. Recursively split top-level AND
2. Sort sub-formulas by heuristic:
   - Strategy 0: by variable count
   - Strategy 1: by clause size (default)
```

**Used in**:
- `lib/include/dfa_combine/dfs_product.cpp` (multiple locations)

**Must keep**: Yes, used for incremental composition

---

### 5. Utility Functions

#### 5.1 Variable Extraction

**Function**: `unsigned int get_var_num(aalta_formula *af)`

**Location**: `af_utils.cpp:554-559`

**Purpose**: Count unique variables in formula

**Used in**: Sorting heuristics for `getAndSubAfs`

**Must keep**: Yes, but can be optimized

---

#### 5.2 Conflict Checking

**Function**: `bool check_conflict(const unordered_set<int> &edge_set)`

**Location**: `af_utils.cpp:249-257`

**Purpose**: Check if edge contains both p and ¬p

**Algorithm**:
```cpp
for (int lit : edge_set) {
    if (edge_set.contains(-lit))
        return true;  // Contradiction
}
return false;
```

**Used in**: `rmnext` preprocessing

**Must keep**: Yes, critical for validity

---

#### 5.3 Edge Set Completion

**Function**: `void fill_in_edgeset(unordered_set<int> &partial, const unordered_set<int> &all_vars)`

**Location**: `af_utils.cpp:259-270`

**Purpose**: Complete partial assignment to full assignment

**Algorithm**:
```cpp
// For each variable not in partial set:
//   - If neither p nor ¬p is assigned, add p (default true)
```

**Used in**: `rmnext` preprocessing

**Must keep**: Yes, required for complete transitions

---

#### 5.4 Empty Trace Acceptance

**Function**: `bool isAccByEmptyTrace(aalta_formula *af)`

**Location**: `af_utils.cpp:478-513`

**Purpose**: Check if formula is satisfied by empty trace (length 0)

**Returns**:
- `true`: True, Release, WNext
- `false`: False, Next, Until, Literal
- Recursive: And (both), Or (either)

**Used in**: State acceptance checking

**Must keep**: Yes, needed for LTLf semantics

---

### 6. Special Constants

**Functions**:
- `aalta_formula *TAIL()`
- `aalta_formula *NOT_TAIL()`

**Purpose**: Singletons for TAIL marker (end of finite trace)

**Used in**: XNF transformation, rmnext

**Must keep**: **Critical** - Fundamental to LTLf semantics

---

### 7. Structural Operations

#### 7.1 Equality/Hashing

**Functions**:
- `bool operator==(const aalta_formula &other)`
- `size_t _hash` (computed from structure)

**Purpose**: Structural equality for canonicalization

**Used in**: `unique()` function, hash maps

**Must keep**: Yes, but can be redesigned (see proposals)

---

#### 7.2 Tree Navigation

**Functions**:
- `aalta_formula *l_af()` - Get left child
- `aalta_formula *r_af()` - Get right child
- `int oper()` - Get operator type
- `int id()` - Get unique ID

**Used in**: Throughout codebase

**Must keep**: Yes, fundamental tree operations

---

#### 7.3 String Conversion

**Functions**:
- `std::string to_string()`
- `std::string print()`

**Purpose**: Debug output, logging

**Used in**: Debug statements

**Must keep**: Yes, but can be simplified

---

## Functionality to Remove

### 1. Tag System

**What**: `_tag` field and all tag manipulation

**Why**: Only used for specific Lydia preprocessing, not in target config

**Action**: Remove completely

---

### 2. Global Caching (current design)

**What**:
- Static `all_afs` hash set (all formulas ever created)
- Per-formula `_unique` pointer
- Per-formula `_simp` cache pointer

**Why**: Not thread-safe, memory never released, couples concerns

**Action**: Replace with external cache manager (see redesign proposals)

---

### 3. Complex Simplify Rules

**What**: Advanced simplification in `simplify_until`, `simplify_release`
- FG detection
- Complex absorption rules
- Nested Until/Release patterns

**Why**: Many rules are redundant after XNF transformation

**Action**: Keep only basic boolean + Next simplification

---

### 4. Unused Utility Functions

**What**:
- `sorted_by_varNum` (duplicated logic)
- `sorted_by_clauseSize` (duplicated logic)
- `add_clauses_set` (unused in target config)
- `get_clauses_size` (unused in target config)
- `filterByCareVarIdSet` (unused in target config)

**Action**: Remove or consolidate

---

### 5. Sat/LtlF-Specific Code

**What**: Functions only used in `lib/include/ltlfsat/`:
- Complex SAT solver integration
- Evidence generation
- COI (Cone of Influence) computation

**Why**: Not used in synthesis path for target config

**Action**: Keep in separate module if needed, or remove

---

## Minimal API Surface

### Essential Public API (after redesign)

```cpp
namespace formula {

// Core AST
class Formula {
public:
    // Construction
    Formula(int op, Formula* left = nullptr, Formula* right = nullptr);
    static Formula* from_string(const std::string& ltlf_str);

    // Accessors
    int oper() const;
    Formula* left() const;
    Formula* right() const;
    int id() const;

    // Transformations (produce new formulas, don't modify this)
    Formula* nnf() const;
    Formula* simplify() const;

    // Comparison
    bool equals(const Formula* other) const;
    size_t hash() const;

    // Debug
    std::string to_string() const;
};

// Special constants
Formula* True();
Formula* False();
Formula* TAIL();
Formula* NOT_TAIL();

// XNF transformation (external function)
Formula* xnf_with_tail(Formula* f);

// Formula progression (external function)
Formula* rmnext(
    Formula* predecessor,
    Formula* edge,
    const std::unordered_set<int>& all_vars
);

// And-decomposition (external function)
std::vector<Formula*> get_and_sub_formulas(Formula* f);

// Utilities (external functions)
bool is_accepted_by_empty_trace(Formula* f);
bool check_conflict(const std::unordered_set<int>& edge_set);
void fill_in_edgeset(
    std::unordered_set<int>& partial,
    const std::unordered_set<int>& all_vars
);

// Cache management (new)
class FormulaCache {
public:
    Formula* get_or_compute(Formula* f, std::function<Formula*()> compute);
    void clear();  // Allow cache clearing
};

} // namespace formula
```

**API design principles**:
1. **Immutable formulas**: Transformations return new formulas
2. **External caching**: Cache managed separately, not embedded in formulas
3. **Clear ownership**: Smart pointers or explicit ownership semantics
4. **Thread-safe**: No global mutable state

---

## Data Flow Diagram

```
Input (LTLf string)
      ↓
   parse()
      ↓
   Formula* (AST)
      ↓
   nnf()
      ↓
   Formula* (NNF)
      ↓
   simplify()
      ↓
   Formula* (Simplified NNF)
      ↓
   xnf_with_tail()
      ↓
   Formula* (XNF with TAIL)  ← Stored as DFA state
      ↓
   rmnext(state, edge)
      ↓
   Formula* (Next state)  ← New DFA state
```

**Key insight**: Each transformation is a **pure function** (input → output), not a mutation.

---

## Usage Statistics

**Call frequency in target config**:

| Function | Call Sites | Frequency | Critical |
|----------|------------|-----------|----------|
| `parse` | 2 | Low | Yes |
| `nnf` | 2 | Low | Yes |
| `simplify` | 1 | Medium | Yes |
| `xnf_with_tail` | 1 | **Very High** | **Critical** |
| `rmnext` | 1 | **Very High** | **Critical** |
| `getAndSubAfs` | 5 | High | Yes |
| `unique` | 100+ | **Very High** | **Critical** |
| `oper`, `l_af`, `r_af` | 500+ | **Very High** | **Critical** |

**Hot paths**:
1. DFA construction: `parse → nnf → simplify → xnf_with_tail` (once per formula)
2. State exploration: `rmnext` (millions of times)
3. Formula comparison: `unique`, `oper`, `l_af`, `r_af` (billions of times)

---

## Memory and Performance

### Current Memory Usage

**Per-formula overhead**:
- 3 pointers (left, right, unique) + 1 pointer (simp) + 1 pointer (tag) = 5 pointers
- 1 int (op) + 1 int (id) + 1 size_t (hash) = 3 ints
- Total: ~5 * 8 + 3 * 4 = **52 bytes per formula**

**Plus global cache**:
- Static `all_afs` grows forever (never cleared)
- Static `f_to_xnf` map grows forever
- Static `f_to_xnfWithTail` map grows forever

**Problem**: Memory leak by design for long-running processes

---

### Critical Performance Requirements

| Operation | Current | Target | Notes |
|-----------|---------|--------|-------|
| Formula construction | ~100ns | <100ns | Hot path |
| `unique()` lookup | ~50ns | <20ns | **Critical** |
| `oper()` access | ~5ns | <5ns | Hot path |
| `l_af()`/`r_af()` | ~5ns | <5ns | Hot path |
| `nnf()` | O(n) | O(n) | Once per formula |
| `xnf_with_tail()` | O(n) cached | O(n) cached | Once per formula |
| `rmnext()` | O(n) | O(n) | **Critical**, called millions of times |
| `simplify()` | O(n log n) | O(n) | Can be optimized |

**n = formula size**

---

## Summary

### Must Keep (Core)

1. **Formula AST structure** with operators True/False/Literal/Not/And/Or/Next/WNext/Until/Release
2. **Parsing** from string
3. **NNF transformation**
4. **XNF with Tail** (THE critical function)
5. **rmnext** (THE critical operation)
6. **Basic simplification** (boolean + Next)
7. **getAndSubAfs** (for incremental composition)
8. **TAIL/NOT_TAIL constants**
9. **Structural operations** (oper, left, right, id)
10. **Utility functions** (check_conflict, fill_in_edgeset, isAccByEmptyTrace)

### Can Remove

1. Tag system (not used in target config)
2. Global caching (redesign to external cache)
3. Complex simplify rules (redundant after XNF)
4. SAT/LtlF-specific functions (not used)
5. Duplicate/unused utility functions

### API Size Reduction

**Current**: ~50 public methods in `aalta_formula` + ~20 functions in `af_utils`

**Proposed**: ~15 public methods in `Formula` + ~10 external functions

**Reduction**: **~70% fewer public API surface**

---

## Next Steps

See `REDESIGN_PROPOSALS.md` for concrete optimization options and implementation strategies.
