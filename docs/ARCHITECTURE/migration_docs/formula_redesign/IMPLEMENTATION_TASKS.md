# Formula Module Rewrite - Implementation Tasks

**Based on**: [FORMULA_REWRITE_DESIGN.md](./FORMULA_REWRITE_DESIGN.md)

**Status**: Ready for Implementation

---

## Task Breakdown

### Phase 1: Foundation (Week 1)

#### Task 1.1: Create Directory Structure

```
include/formula/
  - formula.hpp
  - formula_pool.hpp
  - formula_ops.hpp

src/formula/
  - formula.cpp
  - formula_pool.cpp
  - nnf.cpp
  - xnf.cpp
  - simplify.cpp
  - rmnext.cpp

tests/formula_tests.cpp
```

**Verification**: `ls -R include/formula src/formula`

---

#### Task 1.2: Implement Formula Class (Basic)

**File**: `include/formula/formula.hpp`, `src/formula/formula.cpp`

**Requirements**:
- [ ] Define `OpType` enum class
- [ ] Define `Formula` class with private constructor
- [ ] Implement immutable accessors: `op()`, `left()`, `right()`, `var_id()`, `hash()`
- [ ] Implement type predicates: `is_true()`, `is_false()`, `is_literal()`, etc.
- [ ] Implement `to_string()` method
- [ ] Add `friend class FormulaPool`

**Testing**:
```cpp
FormulaPool pool;
Formula* t = pool.create_true();
EXPECT_EQ(t->op(), Formula::OpType::True);
EXPECT_TRUE(t->is_true());
EXPECT_EQ(t->to_string(), "True");
```

**Time Estimate**: 2-3 hours

---

#### Task 1.3: Implement FormulaPool Class (Skeleton)

**File**: `include/formula/formula_pool.hpp`, `src/formula/formula_pool.cpp`

**Requirements**:
- [ ] Define `FormulaPool` class
- [ ] Implement constructor/destructor
- [ ] Add `unique_table_` (unordered_set)
- [ ] Add `formulas_` (vector<unique_ptr<Formula>>)
- [ ] Implement `create()` method (basic version without optimization)
- [ ] Implement `size()`, `unique_count()`, `total_count()`

**Testing**:
```cpp
FormulaPool pool;
EXPECT_EQ(pool.size(), 0);
Formula* t = pool.create_true();
EXPECT_EQ(pool.size(), 1);
EXPECT_EQ(pool.unique_count(), 1);
```

**Time Estimate**: 3-4 hours

---

#### Task 1.4: Implement Hash Consing

**File**: `src/formula/formula_pool.cpp`

**Requirements**:
- [ ] Implement `compute_hash()` function
- [ ] Update `create()` to search `unique_table_` first
- [ ] Return existing formula if found
- [ ] Add to `unique_table_` if new
- [ ] Test structural sharing

**Testing**:
```cpp
Formula* a = pool.create_variable("a");
Formula* b = pool.create_variable("b");
Formula* and1 = pool.create_and(a, b);
Formula* and2 = pool.create_and(a, b);
EXPECT_EQ(and1, and2);  // Same pointer!
```

**Time Estimate**: 2-3 hours

---

### Phase 2: Variable Management (Week 1-2)

#### Task 2.1: Implement Variable Declaration

**File**: `src/formula/formula_pool.cpp`

**Requirements**:
- [ ] Implement `declare_variables(outputs, inputs)`
- [ ] Implement `declare_outputs(outputs)`
- [ ] Implement `declare_inputs(inputs)`
- [ ] Add `var_names_`, `var_ids_`, `num_outputs_`, `num_inputs_`
- [ ] Add `outputs_declared_`, `inputs_declared_` flags
- [ ] Implement validation (no duplicate names)
- [ ] Implement `is_fully_declared()`

**Testing**:
```cpp
FormulaPool pool;
pool.declare_variables({"s1", "s2"}, {"p1", "p2"});
EXPECT_EQ(pool.get_variable_id("s1"), 0);
EXPECT_EQ(pool.get_variable_id("p1"), 2);
EXPECT_TRUE(pool.is_output_variable(0));
EXPECT_TRUE(pool.is_input_variable(2));
EXPECT_TRUE(pool.is_fully_declared());
```

**Time Estimate**: 3-4 hours

---

#### Task 2.2: Implement Variable Creation

**File**: `src/formula/formula_pool.cpp`

**Requirements**:
- [ ] Implement `create_variable(name)`
- [ ] Check variable declared before creating
- [ ] Return `Literal` formula with correct `var_id`
- [ ] Implement error handling for undeclared variables

**Testing**:
```cpp
FormulaPool pool;
pool.declare_variables({"a"}, {});
Formula* a = pool.create_variable("a");
EXPECT_EQ(a->op(), Formula::OpType::Literal);
EXPECT_EQ(a->var_id(), 0);
```

**Time Estimate**: 2 hours

---

#### Task 2.3: Implement Partition File Loading

**File**: `src/formula/formula_pool.cpp`

**Requirements**:
- [ ] Implement `load_from_partition(file_path)`
- [ ] Parse `.outputs:` line
- [ ] Parse `.inputs:` line
- [ ] Call `declare_variables()`
- [ ] Handle file errors gracefully

**Testing**:
```cpp
FormulaPool pool;
pool.load_from_partition("test.part");
EXPECT_TRUE(pool.is_fully_declared());
```

**Time Estimate**: 2-3 hours

---

#### Task 2.4: Implement Auto-Extraction (Fallback)

**File**: `src/formula/formula_pool.cpp`

**Requirements**:
- [ ] Implement `extract_variables_from_formula(root)`
- [ ] Traverse formula tree recursively
- [ ] Collect all `Literal` names
- [ ] Declare all as outputs
- [ ] Set `inputs_declared_ = true` with empty inputs

**Testing**:
```cpp
FormulaPool pool;
Formula* f = parse_formula("a & b", pool);  // No partition
pool.extract_variables_from_formula(f);
EXPECT_TRUE(pool.is_fully_declared());
EXPECT_EQ(pool.num_outputs(), 2);
EXPECT_EQ(pool.num_inputs(), 0);
```

**Time Estimate**: 2-3 hours

---

### Phase 3: Formula Operations (Week 2)

#### Task 3.1: Implement NNF Transformation

**File**: `src/formula/nnf.cpp`

**Requirements**:
- [ ] Implement `Formula::nnf(pool)` method
- [ ] Handle all operators: Not, And, Or, Next, Until, Release
- [ ] Apply De Morgan's laws
- [ ] Handle double negation
- [ ] Recursively transform children first
- [ ] Return new Formula via `pool->create()`

**Testing**:
```cpp
// !(a & b) → !a | !b
Formula* and_f = pool.create_and(a, b);
Formula* not_f = pool.create_not(and_f);
Formula* nnf_f = not_f->nnf(pool);
EXPECT_EQ(nnf_f->op(), Formula::OpType::Or);
EXPECT_EQ(nnf_f->left()->op(), Formula::OpType::Not);
EXPECT_EQ(nnf_f->right()->op(), Formula::OpType::Not);
```

**Time Estimate**: 4-5 hours

---

#### Task 3.2: Implement Simplification (O(n) version)

**File**: `src/formula/simplify.cpp`

**Requirements**:
- [ ] Implement `Formula::simplify(pool)` method
- [ ] Use `std::unordered_set` for O(n) performance
- [ ] Implement helper functions:
  - [ ] `simplify_and()`: Flatten AND chain, remove conflicts, remove True
  - [ ] `simplify_or()`: Flatten OR chain, remove duplicates, remove False
  - [ ] `simplify_not()`: Double negation, push Not inward
  - [ ] `simplify_next()`: X(True) → True, X(False) → False
  - [ ] `simplify_until()`, `simplify_release()`
- [ ] Apply all optimization rules from [SIMPLIFY_IMPLEMENTATION.md](../formula_module/SIMPLIFY_IMPLEMENTATION.md)

**Testing**:
```cpp
// a & True → a
Formula* and_f = pool.create_and(a, true_f);
Formula* simp = and_f->simplify(pool);
EXPECT_EQ(simp, a);

// a | a → a
Formula* or_f = pool.create_or(a, a);
Formula* simp = or_f->simplify(pool);
EXPECT_EQ(simp, a);
```

**Time Estimate**: 6-8 hours (complex!)

---

#### Task 3.3: Implement XNF Transformation

**File**: `src/formula/xnf.cpp`

**Requirements**:
- [ ] Implement `Formula::xnf_with_tail(pool)` method
- [ ] Push all Next operators to top level
- [ ] Add TAIL markers for finite traces
- [ ] Handle Until/Release transformations
- [ ] Reference [FORMULA_OPERATIONS.md](../formula_module/FORMULA_OPERATIONS.md) for XNF definition

**Testing**:
```cpp
// X(a & b) → Xa & Xb
Formula* and_f = pool.create_and(a, b);
Formula* next_f = pool.create_next(and_f);
Formula* xnf_f = next_f->xnf_with_tail(pool);
EXPECT_EQ(xnf_f->op(), Formula::OpType::And);
EXPECT_EQ(xnf_f->left()->op(), Formula::OpType::Next);
EXPECT_EQ(xnf_f->right()->op(), Formula::OpType::Next);
```

**Time Estimate**: 4-5 hours

---

#### Task 3.4: Implement Formula Progression (rmnext)

**File**: `src/formula/rmnext.cpp`

**Requirements**:
- [ ] Implement `Formula::rmnext(pool, edge, all_vars)` method
- [ ] Handle all operators with edge assignment
- [ ] Implement progression rules from [FORMULA_OPERATIONS.md](../formula_module/FORMULA_OPERATIONS.md)
- [ ] Key transformations:
  - [ ] `X(f) rmnext edge` → `f` with edge applied
  - [ ] `(f1 & f2) rmnext edge` → `(f1 rmnext edge) & (f2 rmnext edge)`
  - [ ] `(f1 | f2) rmnext edge` → `(f1 rmnext edge) | (f2 rmnext edge)`
  - [ ] Variable progression: keep if in edge, remove if not
- [ ] Handle TAIL marker correctly

**Testing**:
```cpp
// X(a) rmnext {a: true} → True
Formula* next_a = pool.create_next(a);
std::unordered_set<int> edge = {a->var_id()};
Formula* prog = next_a->rmnext(pool, edge, all_vars);
EXPECT_TRUE(prog->is_true());
```

**Time Estimate**: 5-6 hours

---

### Phase 4: Integration & Testing (Week 2)

#### Task 4.1: Write Unit Tests

**File**: `tests/formula_tests.cpp`

**Test Categories**:
- [ ] **Variable Management Tests**
  - [ ] Declaration with partition file
  - [ ] Declaration with vectors
  - [ ] Auto-extraction from formula
  - [ ] Variable lookup and info
  - [ ] Error cases (undeclared, duplicates)

- [ ] **Formula Creation Tests**
  - [ ] Basic operators (And, Or, Not, Next, Until, Release)
  - [ ] Canonicalization (structural sharing)
  - [ ] Hash consing verification

- [ ] **Transformation Tests**
  - [ ] NNF: De Morgan's laws, double negation
  - [ ] Simplify: all rules, idempotence
  - [ ] XNF: Next distribution, TAIL markers
  - [ ] rmnext: progression with edges

- [ ] **Edge Case Tests**
  - [ ] Empty formulas
  - [ ] Nested operations
  - [ ] Large formulas (performance)
  - [ ] Error handling

**Time Estimate**: 4-5 hours

---

#### Task 4.2: Integration Tests

**File**: `tests/integration_tests.cpp`

**Requirements**:
- [ ] End-to-end formula pipeline
- [ ] Parse → NNF → Simplify → XNF → rmnext
- [ ] Multiple transformations in sequence
- [ ] Memory leak detection (Valgrind)

**Testing**:
```cpp
TEST(Integration, FullPipeline) {
    FormulaPool pool;
    pool.load_from_partition("test.part");

    Formula* f = parse_formula("!(a & b) | Xc", pool);
    Formula* nnf_f = f->nnf(pool);
    Formula* simp_f = nnf_f->simplify(pool);
    Formula* xnf_f = simp_f->xnf_with_tail(pool);

    EXPECT_NE(xnf_f, nullptr);
}
```

**Time Estimate**: 3-4 hours

---

#### Task 4.3: Performance Benchmarks

**File**: `tools/benchmarks/formula_bench.cpp`

**Requirements**:
- [ ] Benchmark formula creation (throughput)
- [ ] Benchmark canonicalization (deduplication ratio)
- [ ] Benchmark simplify (O(n) verification)
- [ ] Benchmark rmnext (with various edge sizes)
- [ ] Compare with original implementation

**Metrics**:
- Formulas created per second
- Deduplication ratio (unique / total)
- Simplify time vs formula size (should be linear)
- Memory usage

**Time Estimate**: 2-3 hours

---

### Phase 5: Documentation & Cleanup (Week 2)

#### Task 5.1: Add Documentation

**Files**: All header files

**Requirements**:
- [ ] Add Doxygen comments to all public methods
- [ ] Add usage examples in comments
- [ ] Document preconditions/postconditions
- [ ] Document time/space complexity
- [ ] Add README for module

**Time Estimate**: 2-3 hours

---

#### Task 5.2: Code Review & Refactoring

**Requirements**:
- [ ] Review all code for correctness
- [ ] Check for memory leaks (Valgrind)
- [ ] Check for undefined behavior (AddressSanitizer)
- [ ] Refactor ugly code
- [ ] Ensure consistent naming conventions

**Time Estimate**: 2-3 hours

---

#### Task 5.3: Final Testing

**Requirements**:
- [ ] Run all unit tests (pass 100%)
- [ ] Run all integration tests (pass 100%)
- [ ] Run benchmarks (acceptable performance)
- [ ] Test with real formulas from `tools/benchmarks/sm1000/`
- [ ] Compare results with original implementation

**Time Estimate**: 3-4 hours

---

## Task Summary

| Phase | Tasks | Total Time |
|-------|-------|------------|
| Phase 1: Foundation | 4 | 9-13 hours |
| Phase 2: Variable Management | 4 | 9-12 hours |
| Phase 3: Formula Operations | 4 | 19-24 hours |
| Phase 4: Integration & Testing | 3 | 9-12 hours |
| Phase 5: Documentation & Cleanup | 3 | 7-10 hours |
| **Total** | **18** | **53-71 hours** |

**Estimated Timeline**: 2 weeks (full-time)

---

## Dependencies Between Tasks

```
Task 1.1 (Directory Structure)
    │
    ├─▶ Task 1.2 (Formula Class)
    │       │
    │       ├─▶ Task 1.3 (FormulaPool Skeleton)
    │       │       │
    │       │       ├─▶ Task 1.4 (Hash Consing)
    │       │       │       │
    │       │       │       ├─▶ Task 2.1 (Variable Declaration)
    │       │       │       │       │
    │       │       │       │       ├─▶ Task 2.2 (Variable Creation)
    │       │       │       │       │       │
    │       │       │       │       │       ├─▶ Task 2.3 (Partition Loading)
    │       │       │       │       │       │       │
    │       │       │       │       │       │       ├─▶ Task 2.4 (Auto-Extraction)
    │       │       │       │       │       │       │       │
    │       │       │       │       │       │       │       ├─▶ Task 3.1 (NNF)
    │       │       │       │       │       │       │       │       │
    │       │       │       │       │       │       │       │       ├─▶ Task 3.2 (Simplify)
    │       │       │       │       │       │       │       │       │       │
    │       │       │       │       │       │       │       │       │       ├─▶ Task 3.3 (XNF)
    │       │       │       │       │       │       │       │       │       │       │
    │       │       │       │       │       │       │       │       │       │       ├─▶ Task 3.4 (rmnext)
    │       │       │       │       │       │       │       │       │       │       │       │
    │       │       │       │       │       │       │       │       │       │       │       ├─▶ Task 4.1 (Unit Tests)
    │       │       │       │       │       │       │       │       │       │       │       │       │
    │       │       │       │       │       │       │       │       │       │       │       │       ├─▶ Task 4.2 (Integration Tests)
    │       │       │       │       │       │       │       │       │       │       │       │       │       │
    │       │       │       │       │       │       │       │       │       │       │       │       │       ├─▶ Task 4.3 (Benchmarks)
    │       │       │       │       │       │       │       │       │       │       │       │       │       │       │
    │       │       │       │       │       │       │       │       │       │       │       │       │       │       ├─▶ Task 5.1 (Documentation)
    │       │       │       │       │       │       │       │       │       │       │       │       │       │       │       │
    │       │       │       │       │       │       │       │       │       │       │       │       │       │       │       ├─▶ Task 5.2 (Code Review)
    │       │       │       │       │       │       │       │       │       │       │       │       │       │       │       │       │
    │       │       │       │       │       │       │       │       │       │       │       │       │       │       │       │       ├─▶ Task 5.3 (Final Testing)
```

**Critical Path**: Task 1.1 → 1.2 → 1.3 → 1.4 → 2.1 → 2.2 → 3.2 (simplify is most complex) → 4.1 → 5.3

---

## Acceptance Criteria

Each task is complete when:

1. **Code written** according to specification
2. **Tests pass** (including edge cases)
3. **No memory leaks** (Valgrind clean)
4. **No undefined behavior** (ASan clean)
5. **Documentation added** (comments + Doxygen)
6. **Code reviewed** (by self or peer)

---

## Notes for AI Assistant

When implementing, remember:

1. **Immutable formulas**: Never modify `this`, always return new
2. **FormulaPool ownership**: All formulas owned by pool's `formulas_` vector
3. **Canonicalization**: Always search `unique_table_` before creating
4. **Variable ordering**: Outputs first (0..m-1), then inputs (m..m+n-1)
5. **Error handling**: Throw exceptions, don't return null
6. **Testing**: Test both success and failure cases
7. **Performance**: Use `unordered_set` for O(1) lookups in simplify
8. **Documentation**: Comment complex logic (especially simplify and rmnext)

**Reference Documents**:
- [FORMULA_REWRITE_DESIGN.md](./FORMULA_REWRITE_DESIGN.md) - Complete design
- [FLOWCHARTS.md](./FLOWCHARTS.md) - Visual diagrams
- [MEMORY_MANAGEMENT_DECISION.md](../formula_module/MEMORY_MANAGEMENT_DECISION.md) - Memory design
- [ATOMIC_VARIABLE_HANDLING.md](../formula_module/ATOMIC_VARIABLE_HANDLING.md) - Variable design
- [SIMPLIFY_IMPLEMENTATION.md](../formula_module/SIMPLIFY_IMPLEMENTATION.md) - Simplify details
- [FORMULA_OPERATIONS.md](../formula_module/FORMULA_OPERATIONS.md) - Formula operations theory

---

**Ready to start implementation! 🚀**
