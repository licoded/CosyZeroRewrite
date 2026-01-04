# Memory Management Design Decision

## Overview

This document records the **final design decision for memory management** in the formula module redesign, based on understanding of current implementation and user requirements.

**Date**: 2025-01-01
**Status**: Final decision, to be implemented in Proposal B

---

## Key Insights from User

### 1. Current Implementation Analysis

**User's understanding**: Current code manually implements something similar to shared_ptr.

**Actual implementation**: Manual global object pool with permanent ownership:

```cpp
// Current code
aalta_formula *_unique;  // Points to globally unique version
aalta_formula *_simp;    // Points to simplified version

static afp_set all_afs;  // Global set owning ALL formulas (never deleted)
```

**Characteristics**:
- ✅ Allows sharing (via `unique()` method)
- ❌ Never deletes objects (memory leak by design)
- ✅ Fast (no atomic operations)
- ❌ Not thread-safe
- ❌ Memory cannot be reclaimed

**This is**: Manually managed global pool + pointer references, NOT shared_ptr, NOT unique_ptr.

---

### 2. Critical Simplification: Single DFA Context

**User's insight**: "We only need to consider single DFA situation"

**Implication**:
```cpp
void build_single_dfa(Formula* input) {
    // 1. Create all formulas for this DFA
    Formula* xnf = xnf_with_tail(input);

    // 2. Create all states referencing these formulas
    std::vector<DFAState*> states;
    for (...) {
        Formula* state_formula = rmnext(xnf, edge);
        states.push_back(new DFAState(state_formula));
    }

    // 3. Return DFA
    return new DFA(states);
    // DFA has clear lifecycle
    // All formulas created during construction
    // All can be destroyed together when DFA is destroyed
}
```

**Key points**:
- ✅ Single DFA has clear lifecycle
- ✅ All formulas created during DFA construction
- ✅ DFA destruction → destroy all formulas together
- ✅ **No cross-DFA formula sharing needed**

**This simplifies design significantly!**

---

### 3. Sub-formula Sharing is the Priority

**User's insight**: "When we have two formulas/states a and b, the easiest thing to share is their sub-formulas, this should be the primary consideration in design"

**Example**:
```cpp
// Formula: (p ∧ q) ∨ (p ∧ r)
// Tree representation:
//         Or
//        /  \
//    And(p,q) And(p,r)
//     / \      / \
//    p  q     p  r

// Problem: p appears twice!
// Solution: Share the same p object
```

**Without sharing** (wasteful):
```cpp
auto* p1 = new Formula(Literal, "p");  // First p
auto* and1 = new Formula(And, p1, q);

auto* p2 = new Formula(Literal, "p");  // Second p (duplicate!)
auto* and2 = new Formula(And, p2, r);
// ❌ Memory waste
// ❌ Cannot use pointer equality (p1 != p2)
```

**With sharing** (efficient):
```cpp
auto* p = Formula::create(Literal, "p");  // Only one p
auto* and1 = Formula::create(And, p, q);  // References p
auto* and2 = Formula::create(And, p, r);  // References same p!
// ✅ Memory efficient
// ✅ Pointer equality works (and1->left() == and2->left())
```

**This is exactly what current `unique()` method provides!**

---

## Recommended Design

Based on user insights, the recommended design is:

### Architecture

```
┌─────────────────────────────────────────┐
│  FormulaPool (Global, owns everything)  │
│  - Manages lifecycle of ALL formulas     │
│  - Provides deduplication (hash consing) │
│  - Destroyed when DFA is destroyed       │
└─────────────────────────────────────────┘
              │ owns
              ↓
        ┌─────────┐
        │ Formula │ ← Sub-formulas shared via pool
        └─────────┘
              │ referenced by (raw pointers)
              ↓
        ┌─────────┐
        │DFAState │ ← Only references, does NOT own
        └─────────┘
```

### Implementation

```cpp
class Formula {
public:
    // Factory method: create with automatic deduplication
    static Formula* create(int op, Formula* left = nullptr, Formula* right = nullptr) {
        // Check global pool for existing formula
        Formula* key = compute_key(op, left, right);

        auto it = pool_.find(key);
        if (it != pool_.end()) {
            return *it;  // Return existing (sharing!)
        }

        // Create new formula
        Formula* f = new Formula(op, left, right);
        pool_.insert(f);
        return f;
    }

    // Accessors (raw pointers)
    Formula* left() const { return left_; }
    Formula* right() const { return right_; }
    int oper() const { return op_; }

    // Constants (global singletons)
    static Formula* True();
    static Formula* False();
    static Formula* TAIL();
    static Formula* NOT_TAIL();

private:
    Formula(int op, Formula* left, Formula* right)
        : op_(op), left_(left), right_(right) {
        hash_ = compute_hash();
    }

    int op_;
    Formula* left_;   // Raw pointer (owned by pool)
    Formula* right_;  // Raw pointer (owned by pool)
    size_t hash_;

    // Global pool
    static FormulaPool pool_;
};

class FormulaPool {
    std::unordered_set<Formula*, FormulaHash, FormulaEq> formulas_;

public:
    ~FormulaPool() {
        // Delete ALL formulas when pool is destroyed
        for (Formula* f : formulas_) {
            delete f;
        }
    }

    void insert(Formula* f) {
        formulas_.insert(f);
    }

    Formula* find(Formula* key) {
        auto it = formulas_.find(key);
        return (it != formulas_.end()) ? *it : nullptr;
    }

    void clear() {
        // Clear and delete all formulas
        for (Formula* f : formulas_) {
            delete f;
        }
        formulas_.clear();
    }
};

// DFA State: only references formula, does NOT own
class DFAState {
    const Formula* formula_;  // Raw pointer, NON-OWNING
    int id_;

public:
    DFAState(const Formula* f) : formula_(f) {}

    const Formula* formula() const { return formula_; }

    // Destructor does NOT delete formula_
    ~DFAState() {
        // formula_ is owned by FormulaPool, not us!
    }
};
```

### Usage Pattern

```cpp
// Build a DFA
DFA* build_dfa(Formula* input) {
    // Create formula pool for this DFA
    FormulaPool pool;

    // Create formulas (automatically deduplicated)
    Formula* xnf = Formula::create(Next, nullptr, input);
    Formula* p = Formula::create(Literal, "p");
    Formula* and_pq = Formula::create(And, p, q);
    // p is shared wherever needed

    // Create states (reference formulas, don't own)
    std::vector<DFAState*> states;
    for (int edge : all_edges) {
        Formula* next_formula = rmnext(xnf, edge);
        DFAState* state = new DFAState(next_formula);
        // state references formula, but pool owns it
        states.push_back(state);
    }

    // Return DFA (pool embedded in DFA)
    return new DFA(states, pool);
    // When DFA is destroyed, pool is destroyed
    // Pool destructor deletes all formulas
}
```

---

## Design Principles

### 1. Clear Ownership

| Component | Owns? | Responsibility |
|-----------|-------|---------------|
| **FormulaPool** | ✅ Yes | Owns ALL Formula objects |
| **Formula** | ❌ No | Does NOT own left/right (pool does) |
| **DFAState** | ❌ No | Only references Formula, does NOT own |
| **DFA** | ✅ Yes | Owns FormulaPool and all DFAStates |

**Key insight**: Single ownership model (pool owns everything)

---

### 2. Automatic Deduplication

**Mechanism**: `Formula::create()` checks global pool before creating new formula

```cpp
Formula* p = Formula::create(Literal, "p");  // Creates new
Formula* and1 = Formula::create(And, p, q);  // Creates new, uses existing p
Formula* and2 = Formula::create(And, p, r);  // Creates new, reuses p

// and1->left() == and2->left()  (same p object!)
```

**Benefit**: Sub-formula sharing automatic, users don't need to think about it.

---

### 3. Raw Pointers for Non-Owning References

```cpp
class DFAState {
    const Formula* formula_;  // Raw pointer = NOT owning
};
```

**Why raw pointers are OK here**:
- Clear semantics (raw pointer = reference, not ownership)
- No overhead (just 8 bytes)
- No need for smart pointers (ownership managed by pool)

---

### 4. Lifecycle Bound to DFA

```
DFA created
    ↓
FormulaPool created
    ↓
Formulas created (stored in pool)
    ↓
DFAStates created (reference formulas)
    ↓
DFA used
    ↓
DFA destroyed
    ↓
FormulaPool destroyed
    ↓
All formulas deleted
```

**Benefit**: Clear lifecycle, no memory leaks, easy to reason about.

---

## Comparison with Current Implementation

| Aspect | Current | Recommended |
|--------|---------|-------------|
| **Ownership** | Global all_afs (permanent) | Per-DFA FormulaPool (scoped) |
| **Lifecycle** | Never deleted | Deleted with DFA |
| **Sharing** | unique() method | Formula::create() auto-dedup |
| **Memory leak** | ❌ Yes (permanent) | ✅ No (scoped) |
| **Thread safety** | ❌ No | ❌ No (not needed) |
| **Performance** | ✅ Fast | ✅ Fast (same mechanism) |

**Key improvement**: Scoped lifecycle (can delete when DFA done)

---

## Migration from Current Implementation

### Step 1: Replace global all_afs with per-DFA pool

**Current**:
```cpp
static afp_set all_afs;  // Global, permanent

aalta_formula* aalta_formula::unique() {
    if (_unique == NULL) {
        _unique = all_afs.insert(this).first;
    }
    return _unique;
}
```

**New**:
```cpp
class FormulaPool {
    std::unordered_set<Formula*> formulas_;
public:
    Formula* get_or_create(Formula* f);
};

Formula* Formula::create(FormulaPool& pool, int op, Formula* left, Formula* right) {
    Formula* key = /* compute key */;
    Formula* existing = pool.find(key);
    if (existing) return existing;  // Share!

    Formula* f = new Formula(op, left, right);
    pool.insert(f);
    return f;
}
```

### Step 2: Pass pool explicitly

**Current**:
```cpp
Formula* build_formula() {
    Formula* f = new Formula(...);
    return f->unique();  // Uses global all_afs
}
```

**New**:
```cpp
Formula* build_formula(FormulaPool& pool) {
    return Formula::create(pool, And, left, right);
    // Pool passed explicitly
}
```

### Step 3: DFA owns pool

**Current**:
```cpp
DFA* dfa = new DFA(states);
// Formulas leak (never deleted)
```

**New**:
```cpp
DFA* dfa = new DFA(states, pool);
// DFA owns pool
delete dfa;  // → deletes pool → deletes all formulas
```

---

## Why This Design Works

### 1. Matches User Mental Model

- **Sub-formula sharing**: Automatic via `Formula::create()`
- **Single DFA context**: Pool scoped to DFA lifecycle
- **Clear ownership**: Pool owns everything, states reference

### 2. Preserves Current Benefits

- ✅ Fast (no atomic operations)
- ✅ Automatic deduplication
- ✅ Sub-formula sharing
- ✅ Pointer equality works

### 3. Fixes Current Problems

- ❌ Memory leak → ✅ Scoped deletion
- ❌ Global state → ✅ Per-DFA state
- ❌ Cannot reclaim memory → ✅ Delete with DFA

### 4. Simple to Implement

- No smart pointer complexity
- No reference counting
- No atomic operations
- Clear ownership semantics

---

## Examples

### Example 1: Sub-formula Sharing

```cpp
FormulaPool pool;

// Create shared sub-formula
Formula* p = Formula::create(pool, Literal, "p");

// Multiple formulas share p
Formula* and1 = Formula::create(pool, And, p, q);  // Uses p
Formula* and2 = Formula::create(pool, And, p, r);  // Reuses same p

// Verify sharing
assert(and1->left() == and2->left());  // Same p object!
assert(and1->left() == p);
```

### Example 2: DFA Lifecycle

```cpp
DFA* build_dfa() {
    FormulaPool pool;  // Created for this DFA

    // Create formulas (owned by pool)
    Formula* f = Formula::create(pool, And, p, q);

    // Create states (reference formulas)
    DFAState* s1 = new DFAState(f);
    DFAState* s2 = new DFAState(f);

    // DFA owns pool
    return new DFA({s1, s2}, pool);
}

// Use DFA
DFA* dfa = build_dfa();
dfa->run();

// Destroy DFA (destroys pool → deletes formulas)
delete dfa;
```

### Example 3: No Cross-DFA Sharing

```cpp
// DFA 1
DFA* dfa1 = build_dfa();
FormulaPool& pool1 = dfa1->pool();
Formula* f1 = Formula::create(pool1, And, p, q);

// DFA 2 (separate pool)
DFA* dfa2 = build_dfa();
FormulaPool& pool2 = dfa2->pool();
Formula* f2 = Formula::create(pool2, And, p, q);

// f1 and f2 are DIFFERENT objects (different pools)
assert(f1 != f2);
// ✅ This is OK! We don't need cross-DFA sharing.
```

---

## Summary

### Decision

**Use**: Global pool with raw pointers (NOT smart pointers)

### Architecture

```
FormulaPool (per-DFA, owns everything)
    ↓ owns
Formula (raw pointers to children)
    ↓ referenced by
DFAState (raw pointers, non-owning)
```

### Key Points

1. **FormulaPool**: Per-DFA, owns all Formula objects
2. **Formula::create()**: Automatic deduplication (sub-formula sharing)
3. **Raw pointers**: Used for non-owning references (DFAState → Formula)
4. **Scoped lifecycle**: Pool destroyed when DFA destroyed
5. **No cross-DFA sharing**: Each DFA has its own pool

### Benefits

- ✅ Sub-formula sharing automatic
- ✅ Clear ownership semantics
- ✅ Fast (no atomic operations)
- ✅ Memory reclaimable (delete with DFA)
- ✅ Simple implementation
- ✅ Matches user mental model

---

## Related Documentation

- **PROPOSAL_B_ARCHITECTURE.md**: Detailed implementation plan
- **ADVANCED_OPTIMIZATION_TECHNIQUES.md**: Why smart pointers are not needed
- **BDD_CONSIDERATIONS.md**: How this design facilitates future BDD migration
- **CORE_FUNCTIONALITY.md**: What to keep from current implementation

---

**Status**: Final design decision, to be implemented in Proposal B.
