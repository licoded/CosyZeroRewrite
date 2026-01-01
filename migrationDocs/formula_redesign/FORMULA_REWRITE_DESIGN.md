# Formula Module Rewrite - Complete Design Specification

**Version**: 1.1
**Date**: 2025-01-02
**Status**: Ready for Implementation

**Changelog**:
- v1.1 (2025-01-02): Added `pool_index_` member, `operator<`, FormulaEqual pointer check, rmnext specification
- v1.0 (2025-01-01): Initial design

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Architecture Overview](#architecture-overview)
3. [Core Components](#core-components)
4. [Data Flow Diagrams](#data-flow-diagrams)
5. [API Specifications](#api-specifications)
6. [Implementation Details](#implementation-details)
7. [Sequence Diagrams](#sequence-diagrams)
8. [Class Diagrams](#class-diagrams)
9. [Edge Cases & Error Handling](#edge-cases--error-handling)
10. [Testing Strategy](#testing-strategy)

---

## Executive Summary

### Goals

1. **Replace** `aalta_formula` and `af_utils` with clean, modern C++20 design
2. **Keep** all core functionality: NNF, XNF, simplify, rmnext
3. **Improve** performance: O(n²) → O(n) for simplify
4. **Simplify** memory management: FormulaPool with clear ownership
5. **Prepare** for future BDD migration

### Key Design Decisions

| Decision | Rationale | Impact |
|----------|-----------|---------|
| **Immutable formulas** | Safe sharing, thread-safe, easier reasoning | All operations return new formulas |
| **FormulaPool ownership** | Per-DFA scoping, automatic cleanup | No manual memory management |
| **Pre-allocated variables** | BDD-ready, fixed ordering | Two-phase parsing required |
| **Raw pointers** | Performance, user preference | Manual lifetime management via FormulaPool |
| **Hash consing** | Automatic deduplication | Structural uniqueness = pointer equality |

---

## Architecture Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Formula Module                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    FormulaPool (Owner)                      │ │
│  │  ┌─────────────────────────────────────────────────────┐   │ │
│  │  │  Variable Management                                │   │ │
│  │  │  - declare_outputs(outputs)                         │   │ │
│  │  │  - declare_inputs(inputs)                           │   │ │
│  │  │  - get_variable_id(name)                            │   │ │
│  │  └─────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────┐   │ │
│  │  │  Canonicalization Cache (Unique Table)              │   │ │
│  │  │  - create(op, left, right, var_id)                 │   │ │
│  │  │  - Returns existing if duplicate                    │   │ │
│  │  └─────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────┐   │ │
│  │  │  Formula Ownership                                  │   │ │
│  │  │  - vector<unique_ptr<Formula>> formulas_            │   │ │
│  │  │  - All formulas owned by pool                       │   │ │
│  │  └─────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                           │                                       │
│                           │ creates                               │
│                           ▼                                       │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Formula (Immutable)                     │ │
│  │  ┌─────────────────────────────────────────────────────┐   │ │
│  │  │  OpType op_                                         │   │ │
│  │  │  Formula* left_   (reference, not owned)            │   │ │
│  │  │  Formula* right_  (reference, not owned)            │   │ │
│  │  │  int var_id_       (only for Literal)               │   │ │
│  │  │  size_t hash_     (cached for comparison)           │   │ │
│  │  └─────────────────────────────────────────────────────┘   │ │
│  │                                                             │ │
│  │  Operations (all return NEW Formula*):                     │ │
│  │  - nnf()                                                  │ │
│  │  - simplify()                                             │ │
│  │  - xnf_with_tail()                                        │ │
│  │  - rmnext(edge, all_vars)                                 │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Component Relationships

```
┌──────────────┐
│    Parser    │
│ (Input File) │
└──────┬───────┘
       │ parses
       ▼
┌──────────────┐     ┌──────────────┐
│ FormulaPool  │────▶│  Formula     │◀───────────────┐
│ (creates)    │     │  (AST node)  │                  │
└──────────────┘     └──────┬───────┘                  │
                            │                           │
                            │ references                │
                            ▼                           │
                     ┌──────────────┐                  │
                     │  Transform   │                  │
                     │  Operations  │                  │
                     └──────┬───────┘                  │
                            │ creates new              │
                            │ (via FormulaPool)        │
                            └──────────────────────────┘
```

---

## Core Components

### 1. Formula Class

#### Definition

```cpp
namespace formula {

class Formula {
public:
    // Operator types (enum class for type safety)
    enum class OpType {
        True, False,           // Constants
        Not, And, Or,           // Boolean operators
        Next, WeakNext,        // Temporal operators (Strong Next, Weak Next)
        Until, Release,        // Temporal operators
        End,                   // End marker for finite traces (LTLf)
        Literal                // Variable reference
    };

    // ========== Immutable Accessors ==========
    OpType op() const { return op_; }
    Formula* left() const { return left_; }
    Formula* right() const { return right_; }
    int var_id() const { return var_id_; }
    size_t hash() const { return hash_; }

    // ========== Type Predicates ==========
    bool is_true() const { return op_ == OpType::True; }
    bool is_false() const { return op_ == OpType::False; }
    bool is_literal() const { return op_ == OpType::Literal; }
    bool is_not() const { return op_ == OpType::Not; }
    bool is_and() const { return op_ == OpType::And; }
    bool is_or() const { return op_ == OpType::Or; }
    bool is_next() const { return op_ == OpType::Next; }
    bool is_until() const { return op_ == OpType::Until; }
    bool is_release() const { return op_ == OpType::Release; }

    // ========== Structure Predicates ==========
    bool is_binary() const {
        return op_ == OpType::And || op_ == OpType::Or ||
               op_ == OpType::Until || op_ == OpType::Release;
    }
    bool is_unary() const {
        return op_ == OpType::Not || op_ == OpType::Next;
    }
    bool is_temporal() const {
        return op_ == OpType::Next || op_ == OpType::Until ||
               op_ == OpType::Release;
    }

    // ========== Operations (return NEW Formula) ==========
    Formula* nnf(FormulaPool& pool) const;
    Formula* simplify(FormulaPool& pool) const;
    Formula* xnf_with_tail(FormulaPool& pool) const;
    Formula* rmnext(FormulaPool& pool, Formula* edge,
                   const std::unordered_set<int>& all_vars) const;

    // ========== String Representation ==========
    std::string to_string() const;
    std::string to_verbose_string() const;

    // ========== Comparison (for sorting in simplify) ==========
    // Uses pool_index_ for strict weak ordering (see HASH_CONSING_ANALYSIS.md section 11.2.5)
    bool operator<(const Formula& other) const {
        return pool_index_ < other.pool_index_;
    }

private:
    // Private constructor (only FormulaPool can create)
    Formula(OpType op, Formula* left, Formula* right, int var_id, size_t pool_index);

    // ========== Immutable Fields ==========
    OpType op_;           // Operator type
    Formula* left_;       // Left child (or operand for unary)
    Formula* right_;      // Right child (null for unary/constant)
    int var_id_;          // Variable ID (only when op_ == Literal)
    size_t hash_;         // Cached hash value for comparison
    size_t pool_index_;   // Index in FormulaPool (for sorting, see operator<)

    friend class FormulaPool;
};

} // namespace formula
```

#### Key Design Points

1. **Immutable**: All fields are `const` effectively (private, no setters)
2. **Raw pointers**: Performance, managed by FormulaPool
3. **Cached hash**: Computed once in constructor, used for comparisons
4. **Private constructor**: Only FormulaPool can create formulas

---

### 2. FormulaPool Class

#### Definition

```cpp
namespace formula {

class FormulaPool {
public:
    // ========== Construction ==========
    FormulaPool();
    ~FormulaPool();
    FormulaPool(const FormulaPool&) = delete;
    FormulaPool& operator=(const FormulaPool&) = delete;

    // ========== Variable Management ==========
    // Method 1: Load from partition file
    void load_from_partition(const std::string& partition_file);

    // Method 2: Declare from parameter lists
    void declare_variables(const std::vector<std::string>& outputs,
                          const std::vector<std::string>& inputs);

    // Method 3: Declare separately
    void declare_outputs(const std::vector<std::string>& outputs);
    void declare_inputs(const std::vector<std::string>& inputs);

    // Method 4: Auto-extract from formula (testing fallback)
    void extract_variables_from_formula(Formula* root);

    // Validation
    bool is_fully_declared() const;
    int num_variables() const { return var_names_.size(); }
    int num_outputs() const { return num_outputs_; }
    int num_inputs() const { return num_inputs_; }

    // ========== Variable Info ==========
    int get_variable_id(const std::string& name) const;
    std::string get_variable_name(int var_id) const;
    bool is_output_variable(int var_id) const;
    bool is_input_variable(int var_id) const;

    // ========== Formula Creation (Canonicalized) ==========
    Formula* create(OpType op, Formula* left, Formula* right, int var_id = -1);
    Formula* create_variable(const std::string& name);
    Formula* create_true();
    Formula* create_false();
    Formula* create_not(Formula* operand);
    Formula* create_and(Formula* left, Formula* right);
    Formula* create_or(Formula* left, Formula* right);
    Formula* create_next(Formula* operand);
    Formula* create_until(Formula* left, Formula* right);
    Formula* create_release(Formula* left, Formula* right);

    // ========== Resource Management ==========
    void clear();  // Free all formulas
    size_t size() const { return formulas_.size(); }

    // ========== Statistics ==========
    size_t unique_count() const { return unique_table_.size(); }
    size_t total_count() const { return formulas_.size(); }
    double deduplication_ratio() const {
        return static_cast<double>(unique_count()) / total_count();
    }

private:
    // ========== Canonicalization ==========
    struct FormulaHash {
        size_t operator()(Formula* f) const {
            return f->hash();
        }
    };

    // FormulaEqual for hash consing equality check
    // See HASH_CONSING_ANALYSIS.md section 11.2 for detailed explanation
    struct FormulaEqual {
        bool operator()(Formula* a, Formula* b) const {
            // Fast path: pointer same (optimization for stored object comparisons)
            if (a == b) return true;

            // Core logic: structural comparison (hash + op + children + var_id)
            return a->hash() == b->hash() &&
                   a->op_ == b->op_ &&
                   a->left_ == b->left_ &&
                   a->right_ == b->right_ &&
                   a->var_id_ == b->var_id_;
        }
    };

    using UniqueTable = std::unordered_set<Formula*, FormulaHash, FormulaEqual>;

    // ========== Member Variables ==========
    UniqueTable unique_table_;              // Canonicalization cache
    std::vector<std::unique_ptr<Formula>> formulas_;  // Formula ownership

    // Variable management
    std::vector<std::string> var_names_;   // var_names_[id] = name
    std::unordered_map<std::string, int> var_ids_;  // var_ids_[name] = id
    int num_outputs_;
    int num_inputs_;
    bool outputs_declared_;
    bool inputs_declared_;
};

} // namespace formula
```

#### Key Design Points

1. **Non-copyable**: Prevents accidental sharing between DFAs
2. **Canonicalization**: `unique_table_` ensures structural uniqueness
3. **Ownership**: `formulas_` vector owns all Formula objects
4. **Variable scope**: Per-DFA, not global

---

## Data Flow Diagrams

### Formula Creation Flow

```
┌─────────────┐
│ User Code   │
└──────┬──────┘
       │ pool->create(And, f1, f2)
       ▼
┌─────────────────────────────────────┐
│ FormulaPool::create()               │
│ 1. Calculate hash for new formula   │
│ 2. Search unique_table_             │
└──────┬──────────────────────────────┘
       │
       ├─ Found? ──▶ Return existing formula*
       │
       ▼ Not found
┌─────────────────────────────────────┐
│ Create new Formula:                 │
│ 1. new Formula(op, left, right)     │
│ 2. Add to formulas_ (ownership)     │
│ 3. Add to unique_table_ (cache)     │
│ 4. Return new formula*              │
└─────────────────────────────────────┘
```

### Formula Transformation Flow

```
┌─────────────┐
│ Formula f1  │
└──────┬──────┘
       │ f1->simplify(pool)
       ▼
┌─────────────────────────────────────┐
│ Formula::simplify()                 │
│ 1. Compute simplified structure     │
│ 2. Call pool->create() for each    │
│    sub-formula                      │
│ 3. Return new Formula* f2           │
└──────┬──────────────────────────────┘
       │
       ▼
┌─────────────┐
│ Formula f2  │  (NEW formula, f1 unchanged)
└─────────────┘
```

### Variable Declaration Flow

```
Scenario 1: With Partition File

┌──────────────┐
│ Parse .part  │
│ file         │
└──────┬───────┘
       │ extract .outputs and .inputs
       ▼
┌─────────────────────────────────────┐
│ FormulaPool::declare_variables()    │
│ 1. Declare outputs first            │
│    outputs[0] → ID 0                │
│    outputs[1] → ID 1                │
│    ...                              │
│ 2. Then declare inputs              │
│    inputs[0] → ID m                 │
│    inputs[1] → ID m+1               │
│    ...                              │
│ 3. Set flags:                       │
│    outputs_declared_ = true         │
│    inputs_declared_ = true          │
└─────────────────────────────────────┘


Scenario 2: Without Partition (Testing)

┌──────────────┐
│ Parse formula│
│ string       │
└──────┬───────┘
       │ build initial formula tree
       ▼
┌─────────────────────────────────────┐
│ FormulaPool::extract_variables()    │
│ 1. Traverse formula tree            │
│ 2. Collect all literal names        │
│ 3. Treat all as outputs             │
│ 4. No inputs                       │
│ 5. Set flags:                       │
│    outputs_declared_ = true         │
│    inputs_declared_ = true          │
└─────────────────────────────────────┘
```

---

## API Specifications

### Variable Management API

#### `declare_variables()`

```cpp
void FormulaPool::declare_variables(
    const std::vector<std::string>& outputs,
    const std::vector<std::string>& inputs
);
```

**Purpose**: Declare all variables at once from partition file.

**Preconditions**:
- Neither outputs nor inputs declared yet
- All variable names are unique (no overlap between outputs/inputs)
- Variable names are valid identifiers (alphanumeric, underscore)

**Postconditions**:
- All variables assigned IDs: outputs first (0..m-1), then inputs (m..m+n-1)
- `outputs_declared_ == true`
- `inputs_declared_ == true`
- `is_fully_declared() == true`

**Example**:
```cpp
FormulaPool pool;
std::vector<std::string> outputs = {"s1", "s2", "s3"};
std::vector<std::string> inputs = {"p1", "p3", "p5"};

pool.declare_variables(outputs, inputs);

// Variable IDs:
// s1 → 0, s2 → 1, s3 → 2, p1 → 3, p3 → 4, p5 → 5
assert(pool.get_variable_id("s1") == 0);
assert(pool.is_output_variable(0) == true);
assert(pool.is_input_variable(3) == true);
```

**Error Handling**:
- Throws `std::runtime_error` if variables already declared
- Throws `std::invalid_argument` if duplicate names in outputs or inputs

---

#### `create_variable()`

```cpp
Formula* FormulaPool::create_variable(const std::string& name);
```

**Purpose**: Create a literal formula referencing a variable.

**Preconditions**:
- Variable declared (via `declare_variables()` or `declare_outputs()/declare_inputs()`)
- `is_fully_declared() == true` (or auto-extract used)

**Returns**: Canonicalized Formula* with `op_ == OpType::Literal`

**Example**:
```cpp
FormulaPool pool;
pool.declare_variables({"s1", "s2"}, {"p1"});

Formula* s1 = pool.create_variable("s1");  // ID 0
Formula* p1 = pool.create_variable("p1");  // ID 2

assert(s1->op() == Formula::OpType::Literal);
assert(s1->var_id() == 0);
```

**Error Handling**:
- Throws `std::runtime_error` if variable not declared
- Throws `std::runtime_error` if variables not fully declared

---

### Formula Creation API

#### `create_and()`

```cpp
Formula* FormulaPool::create_and(Formula* left, Formula* right);
```

**Purpose**: Create AND formula with canonicalization.

**Behavior**:
1. Compute hash for `(And, left, right)`
2. Search `unique_table_` for existing formula
3. If found, return existing (structural sharing!)
4. If not found, create new and add to cache

**Canonicalization Examples**:
```cpp
Formula* a = pool.create_variable("a");
Formula* b = pool.create_variable("b");

// First creation
Formula* and1 = pool.create_and(a, b);  // Creates new

// Same structure, returns same pointer!
Formula* and2 = pool.create_and(a, b);
assert(and1 == and2);

// Different order (AND is commutative)
Formula* and3 = pool.create_and(b, a);
// Implementation should canonicalize order
assert(and1 == and3);  // If order normalization implemented
```

---

### Transformation API

#### `simplify()`

```cpp
Formula* Formula::simplify(FormulaPool& pool) const;
```

**Purpose**: Apply simplification rules to formula.

**Algorithm** (O(n) using HashSet):
```
simplify(f):
    if f is True or False or Literal:
        return f

    if f is Not(g):
        sg = simplify(g)
        return apply_not_rules(sg)

    if f is And(l, r):
        sl = simplify(l)
        sr = simplify(r)
        return simplify_and(sl, sr)  // O(n) with HashSet

    if f is Or(l, r):
        sl = simplify(l)
        sr = simplify(r)
        return simplify_or(sl, sr)  // O(n) with HashSet

    if f is Next(g):
        sg = simplify(g)
        return simplify_next(sg)

    if f is Until(l, r):
        sl = simplify(l)
        sr = simplify(r)
        return simplify_until(sl, sr)

    if f is Release(l, r):
        sl = simplify(l)
        sr = simplify(r)
        return simplify_release(sl, sr)
```

**Time Complexity**: O(n) where n = formula size
**Space Complexity**: O(n) for HashSet cache

---

## Implementation Details

### NNF Transformation Algorithm

**NNF (Negation Normal Form)**: A formula is in NNF if all negation operators appear only directly in front of atomic propositions (literals).

**Why NNF?**
- Simplifies subsequent transformations (XNF, simplify, rmnext)
- Makes formula structure more regular
- Enables optimization opportunities

**Transformation Strategy**:
1. Push all negation operators inward using De Morgan's laws and duality rules
2. Eliminate double negations
3. Handle LTLf-specific operators (F, G, U, R, X, WX)

**Key Challenge - Finite Traces**:
In LTLf (finite traces), handling negation of Next operators requires special care:

- **Infinite traces (standard LTL)**: `¬X(φ) ≡ X(¬φ)` works perfectly
- **Finite traces (LTLf)**: Need to handle the final position where there is no "next"

**Solution**: Use **End** marker (atomic proposition representing "this is the last position"):

```
¬X(φ) ≡ X(¬φ) ∨ End
```

This covers two cases:
1. There is a next position, and φ is false there: `X(¬φ)`
2. This is the final position (no next exists): `End`

---

#### NNF Transformation Rules

**Base Cases** (already in NNF, return as-is):
```
True        → True
False       → False
Literal(a)  → Literal(a)      // a is atomic variable
Not(a)      → Not(a)           // a is atomic variable (already in NNF)
```

**Double Negation**:
```
Not(Not(φ)) → φ
```

**De Morgan's Laws** (push Not through And/Or):
```
Not(φ ∧ ψ) → (¬φ) ∨ (¬ψ)
Not(φ ∨ ψ) → (¬φ) ∧ (¬ψ)
```

**Temporal Operators - Duality Rules**:

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

**Strong Next (X)** - **LTLf finite trace handling**:
```
Not(X(φ)) → X(¬φ) ∨ End      // End marker for finite traces!
```

**Weak Next (WX)** - **Optional input syntax**:
```
WX(φ) → X(φ)                 // Direct conversion: Weak Next → Strong Next
Not(WX(φ)) → X(¬φ)           // No End needed! (WX is inherently safe for finite traces)
```

**Note on Weak Next (WX)**:
- WX is semantically equivalent to `X(φ) ∨ End` (next φ holds, or this is the last position)
- During parsing, WX is automatically converted to X for internal representation
- The End marker is only added when negating Strong Next: `¬X(φ) → X(¬φ) ∨ End`
- This design simplifies the internal representation while supporting user-friendly WX syntax

---

#### Implementation

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
    }
}

// Main NNF transformation function
Formula* Formula::nnf(FormulaPool& pool) const {
    switch (op_) {
        case OpType::True:
        case OpType::False:
        case OpType::Literal:
            return const_cast<Formula*>(this);  // Already in NNF

        case OpType::Not:
            // Handle negation using helper
            return to_nnf_not(pool, left_);

        case OpType::And:
        case OpType::Or:
        case OpType::Until:
        case OpType::Release:
            // Structure-preserving: recurse on children
            return pool.create(
                op_,
                left_->nnf(pool),
                right_->nnf(pool)
            );

        case OpType::Next:
            // Structure-preserving: recurse on child
            return pool.create_next(left_->nnf(pool));
    }
}
```

---

#### End Marker Implementation

The End marker is a special atomic proposition representing "this is the last position in the trace".

**FormulaPool Support**:
```cpp
class FormulaPool {
public:
    // Create End marker (singleton)
    Formula* create_end() {
        // End is a special marker, treated similarly to True/False
        if (!end_marker_) {
            end_marker_ = create(OpType::End, nullptr, nullptr, -1);
        }
        return end_marker_;
    }

private:
    Formula* end_marker_ = nullptr;  // Singleton End marker
};
```

**OpType Extension**:
```cpp
enum class OpType {
    True, False,
    Not, And, Or,
    Next, Until, Release,
    End,       // End marker for finite traces
    Literal    // Variable reference
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

#### Example Transformations

**Example 1**: Simple De Morgan
```
Input:  !(a & b)
Step 1: Apply to_nnf_not to And
Step 2: Not(a & b) → !a | !b
Output: (!a) | (!b)
```

**Example 2**: Next negation with End
```
Input:  !X(a)
Step 1: Apply to_nnf_not to Next
Step 2: Not(X(a)) → X(!a) | End
Output: X(!a) | End
```

**Example 3**: Complex formula
```
Input:  !(a U X(b))
Step 1: Apply to_nnf_not to Until
Step 2: Not(a U X(b)) → (!a) R (!X(b))
Step 3: Recurse: !X(b) → X(!b) | End
Output: (!a) R (X(!b) | End)
```

**Example 4**: Weak Next (WX) conversion
```
Input (user syntax):  WX(a)
After parsing:        X(a)                 // WX converted to X
After NNF:            X(a)                 // No change needed

Input (user syntax):  !WX(a)
After parsing:        !X(a)                // WX converted to X
After NNF:            X(!a) | End          // Apply Next negation rule
```

---

### XNF Transformation Algorithm

**See Also**: [XNF_TRANSFORMATION.md](./XNF_TRANSFORMATION.md) for complete XNF specification.

**XNF (neXt Normal Form)**: A formula is in XNF if all primitive subformulas `pa(φ)` contain only literals, Strong Next (X), and Weak Next (WX).

**Key Transformation Rules**:
```
xnf(φ₁ U φ₂) = (xnf(φ₂) ∧ ♢true) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))
                                      ^^^^^^^^^^
                    KEEP ORIGINAL - DO NOT RECURSE!

xnf(φ₁ R φ₂) = (xnf(φ₂) ∨ □false) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))
                                      ^^^^^^^^^^
                    KEEP ORIGINAL - DO NOT RECURSE!
```

**Critical Points**:
1. **No Recursion**: U/R inside Next is NOT recursively expanded
2. **♢true ≡ ¬End**: Eventually true = not at the end
3. **□false ≡ End**: Always false = at the end
4. **One-Level Expansion**: Until/Release are expanded only once

**Example**:
```
Input: φ = (¬End ∧ a) U b
xnf(φ) = (b ∧ ¬End) ∨ ((¬End ∧ a) ∧ X((¬End ∧ a) U b))
```

**Complete Specification**: See [XNF_TRANSFORMATION.md](./XNF_TRANSFORMATION.md) for detailed explanation, implementation, and examples.

---

### Formula Progression (rmnext)

**See Also**: [FORMULA_OPERATIONS.md](../formula_module/FORMULA_OPERATIONS.md) for complete rmnext theory and examples.

**Purpose**: `rmnext(predecessor, edge, all_vars)` computes the **next state formula** after applying a transition. This is the critical operation for DFA construction.

**Function Signature**:
```cpp
Formula* Formula::rmnext(FormulaPool& pool, Formula* edge,
                          const std::unordered_set<int>& all_vars) const;
```

**Parameters**:
- `predecessor` (this): Current state formula (**MUST be in XNF format**)
- `edge`: Formula representing the transition (assignment to variables)
- `all_vars`: Set of all variable IDs in the system

**Returns**: New formula for the next state

**Progression Rules by Operator**:

| Operator | Rule | Notes |
|----------|------|-------|
| **Literal p** | `⊤` if p ∈ edge, else `⊥` | Variable satisfied by assignment |
| **Not p** | `⊤` if p ∉ edge, else `⊥` | Negation satisfied |
| **φ ∧ ψ** | `rmnext(φ) ∧ rmnext(ψ)` | Distribute over AND |
| **φ ∨ ψ** | `rmnext(φ) ∨ rmnext(ψ)` | Distribute over OR |
| **X φ** | `φ ∧ ¬End` | Strong Next: must continue |
| **W φ** | `φ ∨ End` | Weak Next: may terminate |
| **End** | `⊥` | End marker: no next state |
| **U, R** | ERROR | Should not appear in XNF |

**Short-circuit Optimizations**:
```
⊥ ∧ ψ → ⊥        (AND short-circuit)
⊤ ∧ ψ → ψ
⊤ ∨ ψ → ⊤        (OR short-circuit)
⊥ ∨ ψ → ψ
```

**Critical Implementation Notes**:
1. **XNF Requirement**: Input MUST be in XNF format (no U/R operators)
2. **End Handling**: End marker becomes False during progression
3. **Edge Representation**: Edge is a formula, typically a conjunction of literals
4. **Variable Lookup**: Use `all_vars` set to check variable presence

**Example**:
```cpp
// Current state: X(p ∨ q)
// Edge: {p, r}  (p=true, r=true, q=false)
// all_vars: {p, q, r}

Formula* next = current->rmnext(pool, edge, all_vars);
// Result: (⊤ ∨ ⊥) ∧ ¬End = ¬End
// Meaning: Trace MUST continue (enforced by Strong Next)
```

**Complete Specification**: See [FORMULA_OPERATIONS.md](../formula_module/FORMULA_OPERATIONS.md) section "Core Operation: rmnext" for:
- Theoretical foundation (FP function from LTLf semantics)
- Detailed examples
- Edge representation
- FormulaProgression utility function

---

### Hash Function

```cpp
size_t FormulaPool::compute_hash(OpType op, Formula* left,
                                Formula* right, int var_id) {
    size_t h = std::hash<int>{}(static_cast<int>(op));

    if (left) {
        h ^= left->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    if (right) {
        h ^= right->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    if (op == OpType::Literal) {
        h ^= std::hash<int>{}(var_id) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    return h;
}
```

### FormulaPool::create() Implementation

```cpp
Formula* FormulaPool::create(OpType op, Formula* left,
                           Formula* right, int var_id) {
    // Step 1: Compute hash for lookup
    size_t h = compute_hash(op, left, right, var_id);

    // Step 2: Create temporary formula for lookup (pool_index = 0)
    Formula key(op, left, right, var_id, h, 0);

    // Step 3: Search in unique table
    auto it = unique_table_.find(&key);
    if (it != unique_table_.end()) {
        return *it;  // Found existing, return it
    }

    // Step 4: Not found, create new formula with unique pool_index
    size_t pool_index = formulas_.size();  // Next available index
    Formula* new_formula = new Formula(op, left, right, var_id, h, pool_index);

    // Step 5: Take ownership
    formulas_.emplace_back(new_formula);

    // Step 6: Add to unique table
    unique_table_.insert(new_formula);

    return new_formula;
}
```

**Key Points**:
1. **Temporary key**: Uses `pool_index = 0` (invalid value, doesn't affect lookup)
2. **Unique index**: `pool_index = formulas_.size()` guarantees uniqueness
3. **Index assignment**: Happens BEFORE adding to `formulas_` vector
4. **Immutable index**: Once assigned, `pool_index` never changes

---

## Sequence Diagrams

### Formula Creation and Canonicalization

```
User          FormulaPool          Formula          UniqueTable
 │                 │                  │                  │
 │ create(And,f1,f2)│                  │                  │
 │────────────────▶│                  │                  │
 │                 │ compute_hash()   │                  │
 │                 │─────────────────▶│                  │
 │                 │                  │                  │
 │                 │  hash value      │                  │
 │                 │◀─────────────────│                  │
 │                 │                  │                  │
 │                 │ find(hash)       │                  │
 │                 │──────────────────│─────────────────▶│
 │                 │                  │                  │
 │                 │  not found       │                  │
 │                 │◀─────────────────│──────────────────│
 │                 │                  │                  │
 │                 │ new Formula(...) │                  │
 │                 │─────────────────▶│                  │
 │                 │                  │                  │
 │                 │  new Formula*    │                  │
 │                 │◀─────────────────│                  │
 │                 │                  │                  │
 │                 │ formulas_.add()  │                  │
 │                 │──────────────────│                  │
 │                 │                  │                  │
 │                 │ insert(new)      │                  │
 │                 │──────────────────│─────────────────▶│
 │                 │                  │                  │
 │                 │  return new*     │                  │
 │                 │◀─────────────────│──────────────────│
 │  Formula*        │                  │                  │
 │◀────────────────│                  │                  │
```

### Simplification Process

```
User          Formula          FormulaPool         Formula
 │                │                   │                │
 │ f->simplify()  │                   │                │
 │───────────────▶│                   │                │
 │                │ is And(l,r)       │                │
 │                │                   │                │
 │                │ l->simplify(p)    │                │
 │                │───────────────────│───────────────▶│
 │                │                   │                │
 │                │  simpl_l          │                │
 │                │◀──────────────────│────────────────│
 │                │                   │                │
 │                │ r->simplify(p)    │                │
 │                │───────────────────│───────────────▶│
 │                │                   │                │
 │                │  simpl_r          │                │
 │                │◀──────────────────│────────────────│
 │                │                   │                │
 │                │ simplify_and()    │                │
 │                │ (uses HashSet)    │                │
 │                │───────────────────│───────────────▶│
 │                │                   │                │
 │                │  result           │                │
 │                │◀──────────────────│────────────────│
 │  result*       │                   │                │
 │◀───────────────│                   │                │
```

---

## Class Diagrams

### Formula Class Structure

```
┌────────────────────────────────────────────────────┐
│                    Formula                         │
├────────────────────────────────────────────────────┤
│ - op_: OpType                                      │
│ - left_: Formula*                                  │
│ - right_: Formula*                                 │
│ - var_id_: int                                     │
│ - hash_: size_t                                    │
├────────────────────────────────────────────────────┤
│ + op(): OpType                                     │
│ + left(): Formula*                                 │
│ + right(): Formula*                                │
│ + var_id(): int                                    │
│ + hash(): size_t                                   │
├────────────────────────────────────────────────────┤
│ + is_true(): bool                                  │
│ + is_false(): bool                                 │
│ + is_literal(): bool                               │
│ + is_not(): bool                                   │
│ + is_and(): bool                                   │
│ + is_or(): bool                                    │
│ + is_next(): bool                                  │
│ + is_until(): bool                                 │
│ + is_release(): bool                               │
├────────────────────────────────────────────────────┤
│ + nnf(pool): Formula*                              │
│ + simplify(pool): Formula*                         │
│ + xnf_with_tail(pool): Formula*                    │
│ + rmnext(pool, edge, vars): Formula*              │
├────────────────────────────────────────────────────┤
│ + to_string(): string                               │
└────────────────────────────────────────────────────┘
```

### FormulaPool Class Structure

```
┌───────────────────────────────────────────────────────────┐
│                    FormulaPool                              │
├───────────────────────────────────────────────────────────┤
│ - unique_table_: UniqueTable                               │
│ - formulas_: vector<unique_ptr<Formula>>                  │
│ - var_names_: vector<string>                               │
│ - var_ids_: unordered_map<string, int>                     │
│ - num_outputs_: int                                        │
│ - num_inputs_: int                                         │
│ - outputs_declared_: bool                                  │
│ - inputs_declared_: bool                                   │
├───────────────────────────────────────────────────────────┤
│ + FormulaPool()                                            │
│ + ~FormulaPool()                                           │
├───────────────────────────────────────────────────────────┤
│ + load_from_partition(path)                                │
│ + declare_variables(outputs, inputs)                       │
│ + declare_outputs(outputs)                                 │
│ + declare_inputs(inputs)                                   │
│ + extract_variables_from_formula(root)                     │
│ + is_fully_declared(): bool                                │
├───────────────────────────────────────────────────────────┤
│ + get_variable_id(name): int                               │
│ + get_variable_name(id): string                            │
│ + is_output_variable(id): bool                             │
│ + is_input_variable(id): bool                              │
├───────────────────────────────────────────────────────────┤
│ + create(op, left, right, var_id): Formula*                │
│ + create_variable(name): Formula*                          │
│ + create_true(): Formula*                                  │
│ + create_false(): Formula*                                 │
│ + create_not(operand): Formula*                            │
│ + create_and(left, right): Formula*                        │
│ + create_or(left, right): Formula*                         │
│ + create_next(operand): Formula*                           │
│ + create_until(left, right): Formula*                      │
│ + create_release(left, right): Formula*                    │
├───────────────────────────────────────────────────────────┤
│ + clear()                                                  │
│ + size(): size_t                                           │
│ + unique_count(): size_t                                    │
│ + total_count(): size_t                                     │
│ + deduplication_ratio(): double                            │
└───────────────────────────────────────────────────────────┘
```

---

## Edge Cases & Error Handling

### 1. Undeclared Variable Access

**Scenario**: User tries to create variable before declaring variables

```cpp
FormulaPool pool;
Formula* f = pool.create_variable("x");  // ERROR!
```

**Handling**:
```cpp
Formula* FormulaPool::create_variable(const std::string& name) {
    if (!is_fully_declared()) {
        throw std::runtime_error(
            "Variables not declared. Call declare_variables() first."
        );
    }

    auto it = var_ids_.find(name);
    if (it == var_ids_.end()) {
        throw std::runtime_error(
            "Variable not declared: " + name
        );
    }

    return create(OpType::Literal, nullptr, nullptr, it->second);
}
```

### 2. Duplicate Formula Creation

**Scenario**: Same formula created multiple times

```cpp
Formula* a = pool.create_variable("a");
Formula* b = pool.create_variable("b");
Formula* and1 = pool.create_and(a, b);
Formula* and2 = pool.create_and(a, b);
assert(and1 == and2);  // Same pointer!
```

**Guarantee**: Structural uniqueness via hash consing

### 3. Variable Name Conflicts

**Scenario**: Outputs and inputs have overlapping names

```cpp
std::vector<std::string> outputs = {"x", "y"};
std::vector<std::string> inputs = {"x", "z"};  // "x" in both!

pool.declare_variables(outputs, inputs);  // ERROR!
```

**Handling**:
```cpp
void FormulaPool::declare_variables(
    const std::vector<std::string>& outputs,
    const std::vector<std::string>& inputs)
{
    if (outputs_declared_ || inputs_declared_) {
        throw std::runtime_error("Variables already declared");
    }

    // Check for duplicates
    std::unordered_set<std::string> seen;
    for (const auto& name : outputs) {
        if (!seen.insert(name).second) {
            throw std::invalid_argument("Duplicate output: " + name);
        }
    }
    for (const auto& name : inputs) {
        if (!seen.insert(name).second) {
            throw std::invalid_argument(
                "Duplicate input (also in outputs): " + name
            );
        }
    }

    // ... proceed with declaration
}
```

### 4. Formula Pool Lifecycle

**Scenario**: DFA destroyed, formulas need cleanup

```cpp
{
    FormulaPool pool;  // Per-DFA scope
    pool.declare_variables(outputs, inputs);
    Formula* f = parse_formula("p1 | p2", pool);
    // ... use formulas
}  // Pool destructor runs, frees all formulas
```

**Guarantee**: RAII - all formulas automatically freed

### 5. Circular References (Impossible)

**Scenario**: Can formulas create cycles?

**Answer**: **No**, due to immutable design

```cpp
// Cannot happen:
// a = create_and(b, c);
// b = create_and(a, d);  // Compilation error!
//
// Why? a is not yet constructed when passed to create_and()
```

**Immutable construction guarantee**:
- Formulas can only reference already-constructed formulas
- No cycles possible
- No reference counting needed

---

## Testing Strategy

### Unit Tests

#### Test 1: Variable Declaration

```cpp
TEST(FormulaPool, DeclareVariables) {
    FormulaPool pool;
    std::vector<std::string> outputs = {"s1", "s2"};
    std::vector<std::string> inputs = {"p1", "p2"};

    pool.declare_variables(outputs, inputs);

    EXPECT_EQ(pool.get_variable_id("s1"), 0);
    EXPECT_EQ(pool.get_variable_id("s2"), 1);
    EXPECT_EQ(pool.get_variable_id("p1"), 2);
    EXPECT_EQ(pool.get_variable_id("p2"), 3);

    EXPECT_TRUE(pool.is_output_variable(0));
    EXPECT_TRUE(pool.is_input_variable(2));
    EXPECT_TRUE(pool.is_fully_declared());
}
```

#### Test 2: Formula Canonicalization

```cpp
TEST(FormulaPool, Canonicalization) {
    FormulaPool pool;
    pool.declare_variables({"a"}, {"b"});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");

    Formula* and1 = pool.create_and(a, b);
    Formula* and2 = pool.create_and(a, b);

    EXPECT_EQ(and1, and2);  // Same pointer!
    EXPECT_EQ(pool.unique_count(), 3);  // a, b, and
}
```

#### Test 3: Simplification

```cpp
TEST(Formula, SimplifyBasic) {
    FormulaPool pool;
    pool.declare_variables({"a"}, {});

    Formula* a = pool.create_variable("a");
    Formula* true_f = pool.create_true();

    // a & True → a
    Formula* and_formula = pool.create_and(a, true_f);
    Formula* simplified = and_formula->simplify(pool);

    EXPECT_EQ(simplified, a);
}
```

#### Test 4: NNF Transformation

```cpp
TEST(Formula, NNFTransformation) {
    FormulaPool pool;
    pool.declare_variables({"a", "b"}, {});

    Formula* a = pool.create_variable("a");
    Formula* b = pool.create_variable("b");

    // !(a & b) → !a | !b
    Formula* and_f = pool.create_and(a, b);
    Formula* not_f = pool.create_not(and_f);
    Formula* nnf_f = not_f->nnf(pool);

    EXPECT_EQ(nnf_f->op(), Formula::OpType::Or);
    EXPECT_EQ(nnf_f->left()->op(), Formula::OpType::Not);
    EXPECT_EQ(nnf_f->right()->op(), Formula::OpType::Not);
}
```

### Integration Tests

#### Test 5: End-to-End Pipeline

```cpp
TEST(FormulaModule, FullPipeline) {
    FormulaPool pool;
    pool.declare_variables({"s1", "s2"}, {"p1"});

    // Parse: "!(p1 & s1) | X(s2)"
    Formula* p1 = pool.create_variable("p1");
    Formula* s1 = pool.create_variable("s1");
    Formula* s2 = pool.create_variable("s2");

    Formula* and_f = pool.create_and(p1, s1);
    Formula* not_f = pool.create_not(and_f);
    Formula* next_s2 = pool.create_next(s2);
    Formula* or_f = pool.create_or(not_f, next_s2);

    // Transform: NNF → Simplify → XNF
    Formula* nnf_f = or_f->nnf(pool);
    Formula* simp_f = nnf_f->simplify(pool);
    Formula* xnf_f = simp_f->xnf_with_tail(pool);

    EXPECT_NE(xnf_f, nullptr);
    EXPECT_TRUE(xnf_f->is_and() || xnf_f->is_or());
}
```

### Property-Based Tests

```cpp
TEST(Formula, SimplifyIdempotent) {
    // Property: simplify(simplify(f)) == simplify(f)
    FormulaPool pool;
    pool.declare_variables({"a", "b", "c"}, {});

    auto test_formula = [&](Formula* f) {
        Formula* s1 = f->simplify(pool);
        Formula* s2 = s1->simplify(pool);
        EXPECT_EQ(s1, s2);
    };

    // Test many random formulas
    for (int i = 0; i < 1000; i++) {
        Formula* f = generate_random_formula(pool, 10);
        test_formula(f);
    }
}
```

---

## Next Steps

1. **Review this document** with user
2. **Confirm design decisions**
3. **Create implementation tasks** (break down by component)
4. **Start implementation** following phase plan

---

## Appendix: Design Decision References

For detailed rationale behind each design decision, see:

- **Memory Management**: [MEMORY_MANAGEMENT_DECISION.md](../formula_module/MEMORY_MANAGEMENT_DECISION.md)
- **Variable Handling**: [ATOMIC_VARIABLE_HANDLING.md](../formula_module/ATOMIC_VARIABLE_HANDLING.md)
- **Architecture**: [PROPOSAL_B_ARCHITECTURE.md](../formula_module/PROPOSAL_B_ARCHITECTURE.md)
- **Simplify Optimization**: [SIMPLIFY_IMPLEMENTATION.md](../formula_module/SIMPLIFY_IMPLEMENTATION.md)
- **Formula Operations**: [FORMULA_OPERATIONS.md](../formula_module/FORMULA_OPERATIONS.md)
