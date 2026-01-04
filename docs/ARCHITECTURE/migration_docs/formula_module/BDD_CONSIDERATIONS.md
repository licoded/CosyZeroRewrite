# BDD Replacement Considerations

## Overview

This document discusses considerations for potentially replacing the current formula tree representation with BDD (Binary Decision Diagram) in the future.

**Context**: Current design uses hash consing (structural sharing) to deduplicate sub-formulas and reduce memory. Future optimization may use BDD for more compact representation and faster operations.

---

## Current Approach: Hash Consing

### How It Works

```cpp
class Formula {
    int op_;
    Formula* left_;
    Formula* right_;
    size_t hash_;

    // Structural uniqueness via canonicalization
    static Formula* create(int op, Formula* left, Formula* right, FormulaCache& cache) {
        Formula* f = new Formula(op, left, right);
        return cache.canonicalize(f);  // Return existing if duplicate
    }
};
```

**Example**:
```cpp
// Create formulas
Formula* f1 = Formula::create(And, p, q, cache);  // Address: 0x1000
Formula* f2 = Formula::create(And, p, q, cache);  // Address: 0x1000 (same!)
Formula* f3 = Formula::create(And, p, r, cache);  // Address: 0x2000 (different)

// Structural uniqueness = pointer equality
assert(f1 == f2);  // True! Same object
assert(f1 == f3);  // False! Different objects
```

**Benefits**:
- Automatic deduplication
- Fast equality check (pointer comparison)
- Reduced memory usage

**Drawbacks**:
- Still tree-based (pointers, memory overhead)
- Operations require tree traversal
- Can't leverage BDD optimizations (reduced order, apply algorithms)

---

## Future Approach: BDD Representation

### What is BDD?

**BDD** (Binary Decision Diagram) is a graph-based representation of Boolean functions:
- Nodes: Variables or boolean constants
- Edges: True/False branches
- Canonical: Reduced and ordered (ROBDD)

**Example**:
```
Formula: (p ∧ q) ∨ (¬p ∧ r)

Tree representation:
         OR
        /  \
      AND   AND
     / \    / \
    p   q ¬p   r

BDD representation:
        p
       / \
      q   r
     / \
    T   F
```

### Potential BDD Implementation

```cpp
class BDDFormula {
    DdNode* bdd_;  // CUDD BDD node
    Cudd* mgr_;    // CUDD manager

public:
    // Operations become BDD operations
    BDDFormula* And(BDDFormula* other) {
        DdNode* result = Cudd_bddAnd(mgr_, bdd_, other->bdd_);
        return new BDDFormula(result, mgr_);
    }

    BDDFormula* Or(BDDFormula* other) {
        DdNode* result = Cudd_bddOr(mgr_, bdd_, other->bdd_);
        return new BDDFormula(result, mgr_);
    }

    // Equality is trivial (BDD is canonical)
    bool equals(BDDFormula* other) {
        return bdd_ == other->bdd_;
    }
};
```

### Benefits of BDD

1. **Canonical by construction**: No need for explicit canonicalization
2. **Efficient operations**: AND/OR implemented via BDD apply
3. **Variable ordering**: Can optimize for specific formulas
4. **Smaller representation**: Often more compact than trees
5. **Fast equivalence checking**: Pointer comparison (already canonical)

### Drawbacks of BDD

1. **External dependency**: Requires CUDD library (or similar)
2. **Learning curve**: BDD semantics different from tree formulas
3. **Variable ordering critical**: Bad ordering → exponential blowup
4. **Not intuitive**: Harder to debug and understand
5. **Overhead**: For small formulas, tree may be faster

---

## Design for Future Migration

### Principle: Don't Over-Abstract

**Your constraint**: "设计上做好抽象 以方便将来替换 但我其实感觉BDD优化去重是过大幅度的变动 所以可能api并不能兼容"

**Translation**: "Design with good abstraction to facilitate future replacement, but BDD optimization is such a major change that API incompatibility is acceptable"

**My recommendation**: **Agree - don't over-abstract**

### Bad Over-Abstraction

```cpp
// DON'T DO THIS: Over-engineered abstraction
class IFormulaRepresentation {
public:
    virtual IFormulaRepresentation* And(IFormulaRepresentation* other) = 0;
    virtual IFormulaRepresentation* Or(IFormulaRepresentation* other) = 0;
    virtual bool equals(IFormulaRepresentation* other) = 0;
    // ... 20+ virtual methods ...
};

class TreeFormula : public IFormulaRepresentation { /* ... */ };
class BDDFormula : public IFormulaRepresentation { /* ... */ };
```

**Why this is bad**:
1. **Virtual overhead**: Every call pays virtual function cost
2. **Complex**: Hard to maintain, many methods to implement
3. **Unclear**: Doesn't actually hide the differences between tree and BDD
4. **YAGNI**: You aren't going to switch representations frequently

### Good: Simple Concrete Design

```cpp
// Current design: Concrete tree-based formula
class Formula {
public:
    // Tree-specific operations
    int oper() const;
    Formula* left() const;
    Formula* right() const;

    // High-level operations (could be tree OR BDD)
    Formula* nnf() const;
    Formula* simplify() const;
    Formula* rmnext(Formula* edge, const VarSet& vars) const;
};

// Future design: Separate BDD-based class
class BDDFormula {
public:
    // BDD-specific operations
    DdNode* bdd() const;
    Cudd* manager() const;

    // High-level operations (same algorithms, different representation)
    BDDFormula* nnf() const;
    BDDFormula* simplify() const;
    BDDFormula* rmnext(BDDFormula* edge, const VarSet& vars) const;
};
```

**Why this is better**:
1. **Concrete**: No virtual overhead, clear what you're using
2. **Separate**: TreeFormula and BDDFormula are independent classes
3. **API incompatibility is OK**: Algorithms similar enough to port, but no need for exact compatibility
4. **YAGNI**: Don't add abstraction layer until you actually need it

---

## Recommended Approach

### Phase 1: Current Redesign (Proposal B)

Implement clean tree-based formula API:
```cpp
namespace formula {

class Formula {
    // Tree-based, immutable
    // Canonicalization via FormulaCache
    // ...
};

} // namespace formula
```

### Phase 2: Evaluate BDD (Future)

After tree-based implementation is working:

1. **Prototype BDD version**:
   - Create `BDDFormula` class alongside `Formula`
   - Implement core operations (And, Or, Not)
   - Benchmark on real workloads

2. **Compare performance**:
   - Memory usage
   - Speed of operations (nnf, simplify, rmnext)
   - Cache behavior

3. **Decision**:
   - If BDD is 5-10× better: Consider migration
   - If BDD is marginal improvement: Keep tree

### Phase 3: Migration (If Warranted)

If BDD evaluation shows significant benefit:

1. **Create parallel implementation**:
   ```
   formula/tree/formula.h      (keep)
   formula/bdd/formula.h       (new)
   ```

2. **Port algorithms**:
   - `nnf()` → BDD version
   - `simplify()` → BDD version
   - `rmnext()` → BDD version

3. **Switch usage**:
   ```cpp
   #ifdef USE_BDD
       using Formula = BDDFormula;
   #else
       using Formula = TreeFormula;
   #endif
   ```

4. **Deprecate tree version** (if BDD proves superior)

---

## Key Considerations for BDD Migration

### 1. Variable Ordering

**Critical**: BDD performance highly dependent on variable ordering

**Decision**: Pre-allocated variables with fixed ordering (see [ATOMIC_VARIABLE_HANDLING.md](./ATOMIC_VARIABLE_HANDLING.md))

**Implementation**:
```
Partition file:
  .inputs: p1 p3 p5
  .outputs: s1 s2 s3

Variable ID assignment:
  p1 → ID 0  (inputs[0])
  p3 → ID 1  (inputs[1])
  p5 → ID 2  (inputs[2])
  s1 → ID 3  (outputs[0])
  s2 → ID 4  (outputs[1])
  s3 → ID 5  (outputs[2])
```

**Example**:
```
Bad ordering: x1, x2, x3, x4, ... (natural order)
Good ordering: inputs first, outputs last (partition-aware) ⭐

For synthesis:
  - Environment variables first
  - System variables last
  - Or vice versa (experiment to find best)
```

**Recommendation**: If implementing BDD, experiment with orderings for LTLf formulas.

### 2. Formula-to-BDD Conversion

**Challenge**: LTLf formulas have temporal operators (X, U, R) which aren't directly representable in BDD

**Solution**: Use BDD for state formulas, handle temporal operators separately:

```cpp
// State formula (no temporal ops): Use BDD
(p ∧ q) ∨ (¬p ∧ r)  →  BDD representation

// Temporal formula: Expand to state formulas + automaton
X φ   →  Transition: state_formula(φ)
φ U ψ →  Automaton with states
```

This is already what `xnf_with_tail()` does - converts temporal to state formulas + next steps.

### 3. TAIL Marker

**Special consideration**: TAIL is not a regular variable

**Options**:
1. **Reserve variable 0 for TAIL**: Treat as special constant in BDD
2. **Encoding**: Add extra dimension to BDD (TAIL vs not-TAIL)

**Recommendation**: Reserve variable 0 (or last variable) for TAIL, handle explicitly in operations.

### 4. Caching Strategy

**Current**: Hash maps from formula* to formula*

**BDD**: BDD is already canonical, no need for structural caching
```cpp
// Current (tree)
Formula* simplify(Formula* f, FormulaCache& cache) {
    if (cache.contains(f)) return cache.get(f);
    // ... compute ...
    cache[f] = result;
    return result;
}

// BDD (no cache needed for structure)
BDDFormula* simplify(BDDFormula* f) {
    // BDD operations automatically canonical
    // Only cache high-level transformations if needed
}
```

---

## API Compatibility Expectations

### What Will Be Similar

**High-level operations**:
```cpp
// Tree
Formula* nnf() const;

// BDD (similar signature)
BDDFormula* nnf() const;
```

**Algorithm structure**:
```cpp
// Both use similar algorithms
Formula* simplify(Formula* f) {
    if (is_literal(f)) return f;
    if (op == And) return simplify_and(f);
    if (op == Or) return simplify_or(f);
    // ...
}
```

### What Will Be Different

**Data access**:
```cpp
// Tree
Formula* left() const;
Formula* right() const;
int oper() const;

// BDD (different access pattern)
DdNode* bdd() const;
Cudd* manager() const;
// No direct left/right access
```

**Construction**:
```cpp
// Tree
Formula* f = Formula::create(And, left, right, cache);

// BDD
BDDFormula* f = BDDFormula::And(left, right, manager);
// OR
DdNode* bdd = Cudd_bddAnd(manager, left->bdd(), right->bdd());
```

**Conclusion**: **API incompatibility is acceptable and expected**

---

## Recommendation

### Short Term (Now)

1. **Implement Proposal B**: Clean tree-based design
   - Don't worry about BDD migration yet
   - Focus on correct, maintainable code
   - Good architecture will make future migration easier

2. **Document assumptions**:
   - Where we assume tree representation
   - Which operations are representation-specific
   - Which are representation-agnostic

### Medium Term (6-12 months)

1. **Prototype BDD version**:
   - Small experimental branch
   - Implement core operations
   - Benchmark against tree version

2. **Evaluate**:
   - Is BDD worth the complexity?
   - Performance improvement on real workloads?
   - Memory usage reduction?

### Long Term (If BDD wins)

1. **Gradual migration**:
   - Keep both implementations during transition
   - Switch via compile-time flag
   - Eventually deprecate tree version

2. **Accept API breakage**:
   - BDD Formula class will have different API
   - Synthesis code will need updates
   - Worth it if performance gain is significant

---

## Summary

| Aspect | Current (Tree) | Future (BDD) |
|--------|----------------|--------------|
| **Representation** | Tree nodes with pointers | Reduced ordered BDD |
| **Canonicalization** | Explicit (hash consing) | Automatic (ROBDD property) |
| **Operations** | Tree traversal | BDD apply algorithms |
| **Equality** | Pointer compare (after canonicalization) | Pointer compare (inherently) |
| **Memory** | Good (deduplication) | Potentially better (compact) |
| **Speed** | Good | Potentially better (for large formulas) |
| **Complexity** | Low (intuitive) | High (learning curve) |
| **Dependencies** | None | CUDD library |

**Recommendation**:
- **Now**: Implement clean tree-based design (Proposal B)
- **Future**: Evaluate BDD through prototyping
- **Migration**: If BDD proves 5-10× better, accept API incompatibility
- **Principle**: Don't over-abstract today for a migration that might not happen

---

## Further Reading

- **BDD basics**: "Binary Decision Diagrams" (Wikipedia, academic papers)
- **CUDD library**: CUDD documentation (University of Colorado)
- **BDD variable ordering**: "Variable Ordering for BDDs" (literature)
- **BDD vs. Trees**: "Decision Diagrams for LTL Model Checking" (academic)
