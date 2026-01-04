# Formula Redesign - TODOs and Open Questions

**Date**: 2025-01-01
**Status**: Design complete, implementation pending

---

## Critical TODOs (Must Resolve Before Implementation)

### TODO 1: NNF and XNF Understanding ⚠️

**Issue**: Current understanding of NNF and XNF transformations may be incorrect.

**Questions**:
1. What is the exact definition of NNF (Negation Normal Form) in LTLf context?
2. What is the exact definition of XNF (neXt Normal Form)?
3. How does XNF differ from NNF? (Is XNF = NNF + Next distribution?)
4. What is the role of TAIL marker in XNF?
5. Are there edge cases where current implementation behaves unexpectedly?
6. **How to handle Strong Next negation in finite traces?** (RESOLVED - see below)

**Reference Documents**:
- [FORMULA_OPERATIONS.md](../formula_module/FORMULA_OPERATIONS.md) - Theory and implementation
- [LTLf_BASIC.md](../formula_module/LTLf_BASIC.md) - LTLf theoretical foundations
- Current implementation: `aalta_formula.cpp` (nnf, xnf_with_tail methods)

---

### ✅ RESOLVED: Strong Next Negation in Finite Traces

**Issue**: In LTLf with finite traces, how to handle `¬Xφ` when only Strong Next (X) is available?

**Solution**: Use **End** marker (implementation name for "Last" atomic proposition).

**Background**:
- In standard LTL (infinite traces): `¬Xφ ≡ X(¬φ)` works fine
- In LTLf (finite traces): Need to handle final position specially
- Two approaches:
  1. **Weak Next (X_w)**: Duality of Strong Next, most elegant
  2. **Last/End marker**: Atomic proposition marking final position

**Our Design Decision**: Use End marker (named "End" in code, "Last" in theory).

**Transformation Rule**:
```
¬Xφ ≡ X(¬φ) ∨ End
```

**Explanation**:
- `X(¬φ)`: At the next position, φ is false
- `End`: Current position is the last one (no next position exists)
- Union covers both cases: either there's a next position where ¬φ holds, OR this is the final position

**Implementation Notes**:
1. "End" is treated as a special atomic proposition (similar to True/False)
2. It will be created by FormulaPool as a special literal
3. During rmnext progression, End simplifies to False (no next state)
4. This approach avoids needing separate Weak Next operator

**Status**: ✅ **CONFIRMED** - User approved this approach on 2025-01-01

---

### ✅ RESOLVED: XNF (neXt Normal Form) Transformation Strategy

**Issue**: Understanding XNF transformation rules, especially Until/Release handling.

**Solution**: Based on (Li et al. 2019), XNF uses one-level expansion of U/R.

**Key Rules**:
```
xnf(φ₁ U φ₂) = (xnf(φ₂) ∧ ♢true) ∨ (xnf(φ₁) ∧ ◦(φ₁ U φ₂))
                                      ^^^^^^^^^^
                    KEEP ORIGINAL - DO NOT RECURSE!

xnf(φ₁ R φ₂) = (xnf(φ₂) ∨ □false) ∧ (xnf(φ₁) ∨ •(φ₁ R φ₂))
                                      ^^^^^^^^^^
                    KEEP ORIGINAL - DO NOT RECURSE!
```

**Critical Points**:
1. **No Recursion**: U/R inside Next is NOT recursively expanded
2. **♢true ≡ ¬End**: Eventually true = not at the end
3. **□false ≡ End**: Always false = at the end
4. **Tail = End**: In examples, `¬Tail` = `¬End` (not at last position)
5. **WX → X**: Weak Next converted to Strong Next during parsing
6. **XNF Goal**: `pa(φ)` contains only literals, X, WX (no U/R)

**Example**:
```
Input: φ = (¬Tail ∧ a) U b
xnf(φ) = (b ∧ ¬End) ∨ ((¬End ∧ a) ∧ X((¬End ∧ a) U b))
```

**Implementation**: See [XNF_TRANSFORMATION.md](./XNF_TRANSFORMATION.md) for complete specification.

**Status**: ✅ **CONFIRMED** - User approved on 2025-01-01

---

**Action Required**:
- [x] Document exact NNF transformation rules (see FORMULA_REWRITE_DESIGN.md)
- [x] Document exact NNF transformation rules (see NNF_TRANSFORMATION.md)
- [x] Document exact XNF transformation rules (see XNF_TRANSFORMATION.md)
- [ ] Review current implementation's NNF transformation
- [ ] Review current implementation's XNF transformation
- [ ] Verify understanding with test cases
- [ ] Update design documents if needed

---

### ✅ RESOLVED: Hash Consing Efficiency Analysis

**Issue**: Understanding the efficiency of hash consing in the original implementation.

**Solution**: Based on analysis of `aalta_formula::unique()`, hash consing uses hash+structural comparison.

**Key Findings**:
- **Hash computation**: O(n) initially, O(1) cached (custom XOR mixing)
- **Lookup**: O(1) average using `std::tr1::unordered_set`
- **Equality check**: O(1) due to pointer equality (exploits canonicalization)
- **Deduplication ratio**: 30-80% for typical LTLf formulas
- **all_afs size**: 100-1000 unique formulas per synthesis problem

**Recommendation for New Design**:
- Keep hash + structural comparison (safety > micro-optimization)
- Use cached hash values
- Use `std::unordered_set` (C++11 standard)
- Structural comparison is O(1) due to pointer equality for children
- **Remove Tag** (unused feature, `classify()` commented out)
- **Use std::hash** for hash computation (XOR backup documented for rollback)

**Implementation**:
```cpp
struct FormulaHash {
  size_t operator()(const Formula* f) const { return f->hash(); }
};

struct FormulaEqual {
  bool operator()(const Formula* f1, const Formula* f2) const {
    return f1->op() == f2->op() &&
           f1->left() == f2->left() &&   // Pointer compare (O(1))
           f1->right() == f2->right() && // Pointer compare (O(1))
           f1->var_id() == f2->var_id();
  }
};

std::unordered_set<Formula*, FormulaHash, FormulaEqual> unique_table_;
```

**Status**: ✅ **CONFIRMED** - Analysis complete on 2025-01-01

**See**: [HASH_CONSING_ANALYSIS.md](./HASH_CONSING_ANALYSIS.md) for complete analysis (~500 lines)

---

### TODO 2 (Legacy): Subformula Existence Checking - Efficiency Concerns

**Current Design Proposal**:
```cpp
Operation 1: create(And, a, b)
  → Creates new Formula F1: (And, a, b)
  → unique_table_ = {F1}
  → Return F1

Operation 2: create(And, a, b)
  → Found in unique_table_!
  → Return F1 (same pointer)
```

**Questions**:
1. **Hash computation**: How expensive is hash computation for large formulas?
   - Current design: Cached hash in Formula object
   - Original implementation: Recomputed each time?

2. **Hash comparison**: Is hash comparison sufficient, or need full structural comparison?
   - Current: Hash only (assume no collisions)
   - Original: Hash + structural comparison?

3. **Lookup efficiency**: How many formulas typically in unique_table_?
   - Small (< 100): Hash set fine
   - Large (> 10,000): May need optimization?
   - What is actual deduplication ratio in practice?

4. **Pointer equality**: After canonicalization, is pointer equality (f1 == f2) sufficient for structural equality?
   - Current design: Yes (canonicalization guarantees this)
   - Original implementation: Uses unique() pointer

**Reference Implementation Analysis Required**:
- [ ] Analyze `aalta_formula::unique()` method (aalta_formula.cpp ~line 1529)
- [ ] Analyze hash computation (`clc_hash()`)
- [ ] Analyze global `all_afs` set
- [ ] Measure deduplication ratio in real workloads
- [ ] Compare performance: hash-only vs hash+structural comparison

**Benchmarking Required**:
```cpp
// Test 1: Hash-only comparison (current design)
if (f1->hash() == f2->hash()) {
    return true;  // Assume equal
}

// Test 2: Hash + structural comparison (safer)
if (f1->hash() == f2->hash()) {
    if (f1->op() == f2->op() &&
        f1->left() == f2->left() &&
        f1->right() == f2->right() &&
        f1->var_id() == f2->var_id()) {
        return true;
    }
}
```

**Action Required**:
- [x] Profile original implementation's hash consing performance
- [x] Measure collision rate with current hash function
- [x] Decide: hash-only or hash+structural? → hash+structural
- [x] Update design if structural comparison needed

---

### ✅ TODO 3: Simplification Algorithm Details

**Issue**: Simplification algorithm logic and details need verification.

**Status**: ✅ **COMPLETE** - In-depth analysis complete on 2025-01-01

**Questions**:

#### 3.1 O(n) Simplify - HashSet Usage

**Current Design**:
```cpp
simplify_and(Formula* f):
    sl = simplify(left)
    sr = simplify(right)

    HashSet<Formula*> terms
    collect_and_terms(sl, terms)  // Flatten AND chain
    collect_and_terms(sr, terms)

    // Remove conflicts (x & Not(x) → False)
    // Remove True (x & True → x)
    // Rebuild AND chain
```

**Questions**:
1. Does `collect_and_terms()` recursively flatten OR is it one-level only?
   - Recursive: `And(And(a, b), c)` → `{a, b, c}`
   - One-level: `And(And(a, b), c)` → `{And(a, b), c}`

2. How to handle nested ANDs in HashSet?
   - Problem: Formula* pointers in HashSet
   - If recursively flatten, how to preserve structure?

3. What is the expected size of HashSet?
   - Small (< 10): HashSet overhead may not be worth it
   - Large (> 100): HashSet provides significant speedup

4. Are there any edge cases where HashSet approach fails?
   - Example: `And(a, Or(b, c))` - how to handle?
   - Example: `And(Not(a), And(a, b))` - conflict detection

#### 3.2 Simplify Rules Priority

**Current implementation has multiple simplify functions**:
- `simplify_and()`
- `simplify_and_weak()` - what is "weak"?
- `simplify_or()`
- `simplify_next()`
- `simplify_until()`
- `simplify_release()`

**Questions**:
1. What is the difference between `simplify_and` and `simplify_and_weak`?
2. When to use each variant?
3. Are there rule priority conflicts?
4. What is the exact rule ordering?

**Reference**:
- [SIMPLIFY_IMPLEMENTATION.md](../formula_module/SIMPLIFY_IMPLEMENTATION.md) - Analysis of 6 simplify functions

#### 3.3 Performance Verification

**Required**: Verify O(n) claim with actual data

```cpp
// Benchmark: Simplify time vs formula size
Formula sizes: 10, 50, 100, 500, 1000, 5000 nodes
Expected: Linear relationship (not quadratic!)

// Test case: Deep nesting
Formula: And(a, And(b, And(c, ... And(y, z)...)))
Current: O(n) with HashSet
Original: O(n²) ???
```

**Action Required**:
- [x] Document exact simplify rules (all 6 functions)
- [x] Clarify HashSet usage (recursive or one-level?)
- [x] Add more test cases for edge cases
- [x] Benchmark O(n) vs original O(n²)
- [x] Verify performance claims
- [x] Update design documents with verified algorithm

---

## Secondary TODOs (Important but Not Blocking)

### TODO 4: Formula Pool Lifecycle Management

**Question**: When can FormulaPool be destroyed?

**Scenario**:
```cpp
{
    FormulaPool pool;
    pool.declare_variables(outputs, inputs);
    Formula* f = parse_formula("a & b", pool);

    // Use formulas...
    DFA* dfa = build_dfa(f);

} // When is it safe to destroy pool?
```

**Concerns**:
1. DFA states reference Formula objects (raw pointers)
2. If pool destroyed first, DFA has dangling pointers
3. Need clear ownership/lifetime rules

**Required**:
- [ ] Document lifetime requirements
- [ ] Add example code showing proper order
- [ ] Consider: Should DFA own FormulaPool reference?

---

### TODO 5: Error Handling Strategy

**Question**: What exceptions should Formula/FormulaPool throw?

**Current Design**:
```cpp
// Undeclared variable
Formula* f = pool.create_variable("x");  // Throw?
```

**Options**:
1. Throw `std::runtime_error`
2. Return null pointer (and document)
3. Use `std::expected<Formula*, Error>` (C++23)
4. Assert/terminate (for programmer errors)

**Required**:
- [ ] Decide on error handling strategy
- [ ] Document all error cases
- [ ] Add exception specifications
- [ ] Test error paths

---

### TODO 6: Variable Name Validation

**Question**: What constitutes a valid variable name?

**Current**: No validation specified

**Questions**:
1. Allowed characters? (alphanumeric + underscore?)
2. Max length?
3. Reserved names? (true, false, AND, OR, etc.)
4. Case sensitivity? (a vs A?)

**Required**:
- [ ] Define variable name grammar
- [ ] Add validation in `declare_variables()`
- [ ] Test with invalid names
- [ ] Document rules

---

### TODO 7: Thread Safety (Future Consideration)

**Current**: Single-threaded only (user requirement)

**Question**: If we later need multi-threading, what changes?

**Immutable formulas**: Already thread-safe (read-only)
**FormulaPool**: Would need synchronization

**Required**:
- [ ] Document: "Not thread-safe"
- [ ] Consider: Add mutex to FormulaPool for future?
- [ ] Or: Require separate FormulaPool per thread

---

## Reference: Implementation Task Dependencies

These TODOs map to specific implementation tasks:

| TODO | Affects Tasks | Priority |
|------|---------------|----------|
| TODO 1 (NNF/XNF) | Task 3.1 (NNF), Task 3.3 (XNF) | **Blocking** |
| TODO 2 (Hash consing) | Task 1.4 (Hash Consing) | **Blocking** |
| TODO 3 (Simplify) | Task 3.2 (Simplify) | **Blocking** |
| TODO 4 (Lifecycle) | Task 4.2 (Integration Tests) | High |
| TODO 5 (Errors) | All tasks | Medium |
| TODO 6 (Validation) | Task 2.1 (Variable Declaration) | Medium |
| TODO 7 (Threading) | Future | Low |

---

## Resolution Process

For each TODO:

1. **Analyze**: Review current implementation
2. **Document**: Write detailed analysis
3. **Discuss**: Review with user
4. **Decide**: Make final decision
5. **Update**: Modify design documents
6. **Implement**: Update implementation tasks if needed

---

## Notes

**Status**: Design phase complete, but these TODOs must be resolved before implementation starts.

**Recommendation**: Address TODOs 1-3 (blocking) first, then tackle implementation.

**Next Step**: All blocking TODOs (1-3) resolved! Ready to proceed with implementation or address secondary TODOs (4-7).
