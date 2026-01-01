# Simplify Series Implementation Analysis

## Overview

The `simplify` series in `aalta_formula` performs formula normalization and optimization by applying logical equivalence rules. This document analyzes the implementation, identifies issues, and provides optimization suggestions.

## Core Simplify Method

```cpp
aalta_formula *aalta_formula::simplify() {
    if (_simp != NULL) return _simp;  // Memoization

    switch (_op) {
        case And:  _simp = simplify_and(_left, _right);   break;
        case Or:   _simp = simplify_or(_left, _right);    break;
        case Next: _simp = simplify_next(_right);         break;
        case Until:_simp = simplify_until(_left, _right); break;
        case Release:_simp = simplify_release(_left,_right); break;
        default:   _simp = unique();                      break;
    }

    _simp->_unique = _simp->_simp = _simp;
    return _simp;
}
```

**Purpose**: Normalize AND/OR to chain structure (flatten tree to list), apply logical simplification rules.

**Design Pattern**: Memoization with lazy evaluation - results cached in `_simp` field.

---

## Individual Simplify Functions

### 1. simplify_next()

**Location**: `aalta_formula.cpp:280`

**Purpose**: Simplify `X φ` (Strong Next operator)

**Rules Implemented**:
```cpp
X False  →  False  // Next of false is false
X φ      →  X φ    // Otherwise, keep structure
```

**Analysis**:
- ✅ Simple and correct
- ⚠️ Commented out `X True → True` optimization (line 286-288)
- ⚠️ Missing `X X φ → X X φ` chaining optimization

**Issues**:
1. **Incomplete optimization**: Should handle `X True` case
2. **No chaining**: `X X φ` could be optimized to `X^2 φ` for efficiency

---

### 2. simplify_or()

**Location**: `aalta_formula.cpp:306`

**Purpose**: Simplify `φ ∨ ψ` (OR operation)

**Rules Implemented**:
```
1. False ∨ φ     →  φ           (Identity elimination)
2. True ∨ φ      →  True        (Domination)
3. φ ∨ ¬φ        →  True        (Contradiction)
4. Flatten: (a ∨ b) ∨ c → a ∨ b ∨ c
5. Deduplication: a ∨ a → a
6. Until absorption: a ∨ (b U ¬a) → True
7. Release absorption: (a R b) ∨ a → a
```

**Algorithm**:
```
Step 1: Split all OR sub-formulas into list
Step 2: Filter trivial cases (False, True, mutex)
Step 3: Sort by pointer address
Step 4: Remove duplicates
Step 5: Apply absorption rules (Until/Release)
Step 6: Rebuild as right-associative chain
```

**Issues**:
1. **Memory leak risk**: `delete[] afp` on multiple return paths
2. **Complex absorption logic**: Lines 356-372 hard to understand
3. **Inefficient sorting**: Sort by pointer address, not semantic meaning
4. **O(n²) conflict check**: Nested loops for conflict detection
5. **Static set usage**: `pos`, `neg` sets reused without clear reset

**Code Quality Problems**:
```cpp
// PROBLEM: Multiple exit points with manual memory management
if (True case) {
    delete[] afp;  // Easy to forget!
    return TRUE();
}
```

---

### 3. simplify_and()

**Location**: `aalta_formula.cpp:401`

**Purpose**: Simplify `φ ∧ ψ` (AND operation)

**Rules Implemented**:
```
1. True ∧ φ      →  φ           (Identity elimination)
2. False ∧ φ     →  False       (Domination)
3. Flatten: (a ∧ b) ∧ c → a ∧ b ∧ c
4. Deduplication: a ∧ a → a
5. Conflict detection: a ∧ ¬a → False
```

**Algorithm**:
```
Step 1: Check if both operands already simplified (fast path)
Step 2: Split all AND sub-formulas into list
Step 3: Filter trivial cases (True, False)
Step 4: Sort by pointer address
Step 5: Remove duplicates
Step 6: Check conflicts (O(n²) pairwise)
Step 7: Rebuild as right-associative chain
```

**Issues**:
1. **Dual-path design**: Has `simplify_and_weak()` variant for pre-simplified inputs
2. **O(n²) conflict detection**: Lines 442-448 nested loops
3. **Memory management**: Manual array allocation/deallocation
4. **Conflict detection limited**: Only checks literal conflicts, not semantic

**Fast Path Optimization**:
```cpp
// Line 403-404: Fast path for already-simplified inputs
if (l->_simp != NULL && r->_simp != NULL)
    return simplify_and_weak(l, r);
```

---

### 4. simplify_and_weak()

**Location**: `aalta_formula.cpp:476`

**Purpose**: Simplify `φ ∧ ψ` when both φ and ψ are already simplified

**Optimization**: Skips redundant simplification, assumes inputs are canonical form

**Algorithm**:
```
Step 1: Check conflicts between all pairs (O(n×m))
Step 2: Merge two sorted lists (like merge sort merge step)
Step 3: Remove True elements
Step 4: Build result based on size
```

**Issues**:
1. **TODO comment in code**: "能否去除list的使用？" (Can we remove list usage?)
2. **Code duplication**: Very similar to `merge_and()` function
3. **Complex merge logic**: Lines 494-524 hard to follow
4. **Multiple passes**: First check conflicts, then merge (could be one pass)

**Comparison with simplify_and**:
| Aspect | simplify_and | simplify_and_weak |
|--------|--------------|-------------------|
| Input assumption | Unsimplified | Pre-simplified |
| Conflict check | O(n²) after sort | O(n×m) before merge |
| Merge strategy | Sort then dedup | Merge sorted lists |
| Use case | General case | Recursive calls |

---

### 5. simplify_until()

**Location**: `aalta_formula.cpp:619`

**Purpose**: Simplify `φ U ψ` (Until operator)

**Rules Implemented**:
```
1. False U φ     →  φ           (Base case)
2. φ U False     →  False       (Never reach)
3. φ U True      →  True        (Always reachable)
4. φ U (φ ∨ ...) →  φ ∨ ...     (Absorption)
5. φ U (φ U ψ)   →  φ U ψ       (Idempotent)
6. φ U (ψ U φ)   →  ψ U φ       (Commutativity variant)
7. φ U (ψ R φ)   →  ψ R φ       (Release conversion)
8. (ψ R φ) U φ   →  φ           (Release absorption)
9. (φ U ψ) U φ   →  ψ U φ       (Associativity variant)
10. (ψ U φ) U φ  →  ψ U φ       (Duplicate right)
11. X φ U φ      →  X φ ∨ φ     (Next absorption)
12. X φ U X ψ    →  X(φ U ψ)    (Next distribution)
13. FG detection: φ U FG(ψ) → FG(ψ) (liveness simplification)
```

**Special Case - FG Detection**:
```cpp
// Lines 624-630
if (r_s->is_GF() || r_s->is_FG()) {
    aalta_formula *FG_core = r_s->_right->_right;
    aalta_formula *ret_af = mk_FG(FG_core);
    simp = ret_af;
}
```

**Issues**:
1. **Complex nested conditions**: 13+ different cases
2. **Recursion in cases**: Lines 642, 648 may cause deep recursion
3. **Deep pointer navigation**: `r_s->_right->_right` fragile
4. **No memoization for containment checks**: `contain()` called multiple times

---

### 6. simplify_release()

**Location**: `aalta_formula.cpp:661`

**Purpose**: Simplify `φ R ψ` (Release operator - dual of Until)

**Rules Implemented**:
```
1. True R φ     →  φ           (Identity)
2. φ R False    →  False       (Bottom)
3. φ R True     →  True        (Top)
4. φ R (φ ∧ ...)→  φ ∧ ...     (Absorption)
5. (φ ∨ ...) R φ→  φ           (Absorption variant)
6. φ R (φ R ψ)  →  φ R ψ       (Idempotent)
7. φ R (ψ R φ)  →  ψ R φ       (Commutativity)
8. φ R (ψ U φ)  →  ψ U φ       (Until conversion)
9. (ψ U φ ∨ ...) R φ → φ       (Until absorption)
10. (φ R ψ) R φ →  ψ R φ       (Associativity)
11. (ψ R φ) R φ →  ψ R φ       (Duplicate right)
12. ¬φ R φ      →  False R φ   (Contradiction)
```

**Issues**:
1. **TODO comment**: Lines 684-685 ask if atomic-only check should extend to formulas
2. **Commented optimization**: Line 681-682 Next distribution commented out
3. **Incomplete TODO**: Line 689 mentions more optimization needed
4. **Asymmetry with simplify_until**: Missing some optimizations

---

## Helper Functions

### split()

**Location**: `aalta_formula.cpp:1254`

**Purpose**: Flatten tree structure into list of operands

**Algorithm**:
```cpp
// Input: (a ∧ (b ∧ c)) ∧ d
// Output: [a, b, c, d]

Non-recursive iterative traversal using stack:
1. While current node has target op:
   - Push right child to stack
   - Continue to left child
2. When current node is not target op:
   - Simplify if requested
   - Add to result list
   - Pop from stack
```

**Issues**:
1. **Mutates input**: Modifies formula during traversal
2. **No const correctness**: `this` not const-qualified
3. **Inefficient**: Creates temporary `store` list

---

### contain()

**Location**: `aalta_formula.cpp:1310, 1339`

**Purpose**: Check if formula contains specific sub-formula at specific position

**Positions**:
- `Left`: Formula's left child equals target
- `Right`: Formula's right child equals target
- `All`: Any descendant equals target

**Issues**:
1. **Deep equality check**: Uses `unique()` pointer comparison, may miss structurally equal formulas
2. **Recursive**: No depth limit, could stack overflow
3. **Overloaded**: Two versions with different signatures

---

### mutex()

**Location**: `aalta_formula.cpp:1286`

**Purpose**: Check if formula contains `a` and `¬a` (contradictory literals)

**Returns**: `true` if formula contains both a literal and its negation

**Issues**:
1. **Only checks literals**: Doesn't detect `a ∧ b` vs `¬a ∨ ¬b` contradictions
2. **Side effects**: Modifies `pos` and `neg` sets
3. **Unclear semantics**: Name suggests mutual exclusion, actually checks contradiction

---

## Overall Code Issues

### 1. Memory Management

**Problems**:
- Raw pointers with manual `new[]`/`delete[]`
- Multiple exit paths make it easy to leak memory
- No RAII patterns

**Example**:
```cpp
aalta_formula **afp = new aalta_formula*[af_list.size () + 1];
// ... many return paths ...
delete[] afp;  // Easy to miss!
```

**Impact**: Memory leaks, crashes from double-free

---

### 2. Code Duplication

**Problem Areas**:
- `simplify_and_weak()` and `merge_and()` share 80% logic
- `simplify_until()` and `simplify_release()` have parallel structure
- Multiple OR/AND building patterns repeated

**Example**:
```cpp
// In simplify_and_weak (lines 538-543)
for (; !af_list.empty (); af_list.pop_back ()) {
    r = aalta_formula (And, af_list.back (), r).unique ();
    r->_simp = r;
}
ret = aalta_formula (And, l, r).unique ();
ret->_simp = ret;

// Similar pattern in simplify_or, merge_and, etc.
```

---

### 3. Global Mutable State

**Static Variables**:
```cpp
static afp_set all_afs;        // All formulas ever created
static aalta_formula *_TRUE;   // Singleton True
static aalta_formula *_FALSE;  // Singleton False
static hash_map<std::string, int> ids;  // Name to ID mapping
```

**Issues**:
1. **Not thread-safe**: Global state limits parallelization
2. **Memory never released**: Formulas accumulate forever
3. **Testing difficulty**: Hard to isolate tests with shared state

---

### 4. Complex Boolean Logic

**Example from simplify_or()**:
```cpp
// Lines 369-372
for (n = i, i = -1, j = 0; j <= n; ++j)
    if ((afp[j]->_op != Release || s1.find (afp[j]->_right) == s1.end ())
        && s2.find (afp[j]) == s2.end ())
      afp[++i] = afp[j];
```

**Problems**:
- Multiple nested conditions
- Unclear variable names (`i`, `j`, `n`, `s1`, `s2`)
- No comments explaining the logic

---

### 5. Performance Issues

**Inefficiencies**:
1. **O(n²) algorithms**: Conflict checking, duplicate detection
2. **Sorting by pointer address**: Not semantically meaningful
3. **Multiple passes**: Split → Filter → Sort → Dedup → Merge
4. **No move semantics**: Copying formulas everywhere
5. **Hash recomputation**: Could cache hash values

**Example**:
```cpp
// Line 354: Sort by pointer address (not semantically meaningful)
std::sort (afp, afp + n);
```

---

### 6. Lack of Encapsulation

**Problems**:
- Public access to internal pointers (`l_af()`, `r_af()`)
- Direct manipulation of `_simp`, `_unique` fields
- No invariants enforced

**Example**:
```cpp
// Anyone can call:
formula->_simp = something_else;  // Breaks caching!
```

---

### 7. Tag Management Complexity

**Purpose**: Track Until nesting for XNF transformation

**Implementation**:
```cpp
typedef std::list<aalta_formula*> tag_t;
tag_t *_tag;  // Per-formula tag
```

**Issues**:
1. **Complex lifetime**: Tags allocated/deleted manually
2. **Hashing complexity**: Custom hash function for tags
3. **Unclear semantics**: What does tag represent exactly?

---

## Optimization Suggestions

### 1. Memory Management - Use Smart Pointers

**Current**:
```cpp
aalta_formula **afp = new aalta_formula*[af_list.size () + 1];
// ... use afp ...
delete[] afp;
```

**Proposed**:
```cpp
std::vector<std::unique_ptr<aalta_formula>> af_list;
// ... automatic cleanup ...
```

**Benefits**:
- No manual memory management
- Exception-safe
- Clear ownership

---

### 2. Use std::vector Instead of Raw Arrays

**Current**:
```cpp
aalta_formula **afp = new aalta_formula*[af_list.size () + 1];
```

**Proposed**:
```cpp
std::vector<aalta_formula*> afp;
afp.reserve(af_list.size());
```

**Benefits**:
- Bounds checking
- Size tracking
- Iterator support
- No manual delete[]

---

### 3. Eliminate Code Duplication

**Extract Common Pattern**:
```cpp
// Helper function to build chain from list
template<typename Op>
aalta_formula* build_chain(
    std::vector<aalta_formula*>& elems,
    Op op
) {
    if (elems.empty()) return identity_element(op);
    if (elems.size() == 1) return elems[0];

    aalta_formula* result = elems[0];
    for (size_t i = 1; i < elems.size(); ++i) {
        result = aalta_formula(op, elems[i], result).unique();
    }
    return result;
}
```

---

### 4. Use HashSet for O(1) Lookup

**Current**: O(n²) conflict detection
```cpp
for (i = 0; i < n; ++i)
    for (j = i + 1; j < n; ++j)
        if (is_conflict(afp[i], afp[j]))
            return FALSE();
```

**Proposed**: O(n) with hash set
```cpp
std::unordered_set<aalta_formula*, af_prt_hash, af_prt_eq> seen;
for (auto* af : af_list) {
    if (seen.contains(get_negation(af)))
        return FALSE();
    seen.insert(af);
}
```

---

### 5. Separate Parsing from Simplification

**Current**: Tags embedded in formula structure

**Proposed**:
- Formula tree is pure (no tags)
- Separate context for Until nesting tracking
- Simplification is a pure function

**Benefits**:
- Cleaner separation of concerns
- Easier to test
- No tag management complexity

---

### 6. Add Formula Visitor Pattern

**Purpose**: Eliminate switch statements

**Current**:
```cpp
switch (_op) {
    case And: _simp = simplify_and(_left, _right); break;
    case Or: _simp = simplify_or(_left, _right); break;
    // ...
}
```

**Proposed**:
```cpp
class SimplifyVisitor {
    aalta_formula* visit(And* and) { return simplify_and(and); }
    aalta_formula* visit(Or* or) { return simplify_or(or); }
    // ...
};
```

---

### 7. Immutable Formula Design

**Current**: Mutable `_simp`, `_unique` fields

**Proposed**:
```cpp
class Formula {
    // Immutable fields only
    const OpKind op;
    const std::shared_ptr<Formula> left;
    const std::shared_ptr<Formula> right;

    // Simplification returns new formula
    std::shared_ptr<Formula> simplify() const {
        // Pure function, no mutation
    }
};
```

**Benefits**:
- Thread-safe
- Easier to reason about
- Can safely share sub-formulas

---

### 8. Lazy Evaluation with Memoization

**Current**: `_simp` cache per formula

**Proposed**:
```cpp
class SimplifyCache {
    std::unordered_map<
        const Formula*,
        std::shared_ptr<Formula>
    > cache_;

public:
    std::shared_ptr<Formula> get_or_compute(
        const Formula* f,
        auto simplifier
    ) {
        if (auto it = cache_.find(f); it != cache_.end())
            return it->second;

        auto result = simplifier(f);
        cache_[f] = result;
        return result;
    }
};
```

**Benefits**:
- Cache can be cleared
- Per-context caching (not global)
- Explicit cache invalidation

---

### 9. Add Formula Normalization

**Purpose**: Canonical representation for structural equality

**Algorithm**:
```
1. Flatten AND/OR to sorted lists
2. Sort by hash/ID
3. Remove duplicates
4. Normalize negation (push to literals)
5. Apply simplification rules
```

**Benefits**:
- Structural hashing possible
- Faster equality checks
- Can use pointer equality

---

### 10. Use Expression Templates

**Purpose**: Build formulas without intermediate allocations

**Current**:
```cpp
// Creates temporary for each sub-expression
auto f1 = new aalta_formula(And, a, b);
auto f2 = new aalta_formula(And, f1, c);
```

**Proposed** (C++):
```cpp
// Expression template builds tree in one allocation
auto f = (a && b) && c;  // Builds formula_t<And, formula_t<And, a, b>, c>
```

---

## Redesign Recommendations

### Architecture Changes

1. **Separate concerns**:
   - Formula AST (immutable)
   - Builder (for construction)
   - Simplifier (for normalization)
   - Equality checker (for comparison)

2. **Use modern C++**:
   - Smart pointers for memory
   - Standard containers
   - Move semantics
   - Const correctness

3. **Remove global state**:
   - Per-context formula pools
   - Explicit cache management
   - Thread-local caches if needed

4. **Add testing infrastructure**:
   - Property-based testing
   - Round-trip tests (parse → simplify → print)
   - Equivalence testing

---

## Conclusion

The current simplify implementation has several significant issues:

**Critical Issues**:
1. ❌ Memory management (raw pointers, manual delete)
2. ❌ Global mutable state (not thread-safe)
3. ❌ Code duplication (simplify_and_weak vs merge_and)

**Important Issues**:
4. ⚠️ Performance (O(n²) algorithms)
5. ⚠️ Complexity (hard to understand/maintain)
6. ⚠️ Tag management (complex error-prone)

**Minor Issues**:
7. ℹ️ Incomplete optimizations (commented code)
8. ℹ️ Limited conflict detection
9. ℹ️ No move semantics

**Recommended Approach**:
1. Phase 1: Add RAII wrappers (no logic changes)
2. Phase 2: Extract common patterns (reduce duplication)
3. Phase 3: Redesign with immutable formulas
4. Phase 4: Add comprehensive tests

The code works but is showing its age - it predates modern C++ practices and would benefit significantly from a redesign using RAII, smart pointers, and cleaner separation of concerns.
