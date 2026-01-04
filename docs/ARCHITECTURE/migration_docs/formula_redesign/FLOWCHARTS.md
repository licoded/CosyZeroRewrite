# Formula Module - Flowcharts and Visual Diagrams

**Companion to**: [FORMULA_REWRITE_DESIGN.md](./FORMULA_REWRITE_DESIGN.md)

---

## Table of Contents

1. [Formula Lifecycle](#formula-lifecycle)
2. [Transformation Pipeline](#transformation-pipeline)
3. [Variable Declaration Flows](#variable-declaration-flows)
4. [Canonicalization Process](#canonicalization-process)
5. [Simplification Algorithm](#simplification-algorithm)
6. [Memory Management](#memory-management)

---

## Formula Lifecycle

### From Input to DAG

```
┌─────────────────┐
│ Input Formula   │
│ "!(a & b) | Xc" │
└────────┬────────┘
         │ parse()
         ▼
┌──────────────────────────────────────────────────────────┐
│                  Initial AST Tree                        │
│                                                          │
│                        Or                                │
│                      /     \                             │
│                    Not      Next                          │
│                   /  \       |                           │
│                 And    ""    c                            │
│                /  \                                        │
│              a     b                                       │
└──────────────────────────────────────────────────────────┘
         │
         │ immutable, hash-consed
         ▼
┌──────────────────────────────────────────────────────────┐
│              After Hash Consing                          │
│  - Each unique structure has one pointer                │
│  - Structural sharing enabled                           │
│  - Example: If "a" appears twice, same pointer used     │
└──────────────────────────────────────────────────────────┘
         │
         │ transformations (nnf, simplify, xnf)
         ▼
┌──────────────────────────────────────────────────────────┐
│              Transformed Formula                         │
│  - New Formula objects created                           │
│  - Old Formula objects unchanged                         │
│  - Both coexist in FormulaPool                          │
└──────────────────────────────────────────────────────────┘
         │
         │ unused formulas reclaimed
         ▼
┌──────────────────────────────────────────────────────────┐
│             FormulaPool Destruction                      │
│  - All Formula objects freed                            │
│  - No memory leaks                                      │
└──────────────────────────────────────────────────────────┘
```

---

## Transformation Pipeline

### Complete Formula Processing

```
┌──────────────┐
│ Parse String │
│ "!(p & q)U r"│
└──────┬───────┘
       │
       ▼
┌─────────────────────────────────────────────────────────┐
│              Initial Formula                            │
│  Op: Until                                              │
│  Left: Not(And(p, q))                                  │
│  Right: r                                              │
└──────┬──────────────────────────────────────────────────┘
       │
       │ nnf()
       ▼
┌─────────────────────────────────────────────────────────┐
│              After NNF                                  │
│  Op: Until                                              │
│  Left: Or(Not(p), Not(q))  ← De Morgan's Law           │
│  Right: r                                              │
└──────┬──────────────────────────────────────────────────┘
       │
       │ simplify()
       ▼
┌─────────────────────────────────────────────────────────┐
│              After Simplify                             │
│  Op: Until                                              │
│  Left: Or(Not(p), Not(q))  ← Already simplified        │
│  Right: r                                              │
│  - No redundant formulas                                │
│  - No False/True where redundant                        │
└──────┬──────────────────────────────────────────────────┘
       │
       │ xnf_with_tail()
       ▼
┌─────────────────────────────────────────────────────────┐
│              After XNF                                  │
│  Op: Until                                              │
│  Left: Or(Not(p), Not(q))                              │
│  Right: r                                              │
│  - All Next operators at top level                     │
│  - TAIL markers added for finite traces                │
└──────┬──────────────────────────────────────────────────┘
       │
       │ Ready for rmnext()
       ▼
┌─────────────────────────────────────────────────────────┐
│           Ready for DFA Construction                   │
│  - Formula in XNF form                                  │
│  - Can compute successors via rmnext()                  │
└─────────────────────────────────────────────────────────┘
```

---

## Variable Declaration Flows

### Scenario 1: Normal Use (With Partition File)

```
┌─────────────────┐
│ Partition File  │
│ .outputs: s1 s2 │
│ .inputs: p1 p2  │
└────────┬────────┘
         │ parser reads file
         ▼
┌─────────────────────────────────────────────────────────┐
│         Step 1: Extract Outputs                          │
│  declare_outputs({"s1", "s2"})                          │
│                                                          │
│  Internal state:                                         │
│    var_names_ = ["s1", "s2"]                            │
│    var_ids_ = {"s1":0, "s2":1}                          │
│    num_outputs_ = 2                                     │
│    outputs_declared_ = true                             │
└─────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│         Step 2: Extract Inputs                           │
│  declare_inputs({"p1", "p2"})                           │
│                                                          │
│  Internal state:                                         │
│    var_names_ = ["s1", "s2", "p1", "p2"]                │
│    var_ids_ = {"s1":0, "s2":1, "p1":2, "p2":3}         │
│    num_inputs_ = 2                                      │
│    inputs_declared_ = true                              │
└─────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│         Step 3: Validation                               │
│  is_fully_declared() == true                            │
│  ✓ Can now create formulas                              │
└─────────────────────────────────────────────────────────┘
```

### Scenario 2: Testing Use (No Partition File)

```
┌─────────────────┐
│ Formula String  │
│ "(a & b) | Xc"  │
└────────┬────────┘
         │ parser builds initial tree
         ▼
┌─────────────────────────────────────────────────────────┐
│         Step 1: Build Initial Formula                   │
│  - Parse formula without variable declaration           │
│  - Creates temporary literals (undeclared)              │
└─────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│         Step 2: Extract Variables                        │
│  extract_variables_from_formula(root)                   │
│                                                          │
│  Traverses tree:                                         │
│    Or → And → a, b                                      │
│         → Next → c                                      │
│                                                          │
│  Extracts: {"a", "b", "c"}                               │
└─────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│         Step 3: Declare as Outputs                       │
│  - Treat all as outputs (system variables)              │
│  - No inputs (environment variables)                     │
│                                                          │
│  Internal state:                                         │
│    var_names_ = ["a", "b", "c"]                         │
│    var_ids_ = {"a":0, "b":1, "c":2}                    │
│    num_outputs_ = 3                                     │
│    num_inputs_ = 0                                      │
│    outputs_declared_ = true                             │
│    inputs_declared_ = true                              │
└─────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│         Step 4: Rebuild Formulas                         │
│  - Replace temporary literals with proper variables    │
│  - All formulas now have valid var_id                  │
└─────────────────────────────────────────────────────────┘
```

---

## Canonicalization Process

### Hash Consing in Detail

```
User calls: pool->create(And, f_a, f_b)
        │
        ▼
┌───────────────────────────────────────┐
│  Compute hash for (And, f_a, f_b)     │
│                                       │
│  hash = hash(And) ^                   │
│          f_a->hash ^                  │
│          f_b->hash                    │
└───────────────┬───────────────────────┘
                │
                ▼
┌───────────────────────────────────────┐
│  Search in unique_table_              │
│                                       │
│  for (existing : unique_table_) {     │
│      if (existing->hash == hash &&    │
│          existing->op == And &&       │
│          existing->left == f_a &&     │
│          existing->right == f_b)      │
│          return existing;  // FOUND!  │
│  }                                   │
└───────────────┬───────────────────────┘
                │
       Found?    │    Not Found?
       │         │         │
       ▼         │         ▼
    Return    │    ┌─────────────────┐
  existing*   │    │ Create Formula  │
       │       │    │ new Formula(...)│
       │       │    └────────┬────────┘
       │       │             │
       │       │             ▼
       │       │    ┌─────────────────┐
       │       │    │ Take Ownership  │
       │       │    │ formulas_.push  │
       │       │    └────────┬────────┘
       │       │             │
       │       │             ▼
       │       │    ┌─────────────────┐
       │       │    │ Cache it        │
       │       │    │ unique_table_   │
       │       │    │    .insert(new) │
       │       │    └────────┬────────┘
       │       │             │
       │       │             ▼
       │       │       Return new*
       │       │             │
       └───────┴─────────────┘
                │
                ▼
         User gets Formula*
```

### Example: Structural Sharing

```
Initial State:
  unique_table_ = {}

Operation 1: create(And, a, b)
  → Creates new Formula F1: (And, a, b)
  → unique_table_ = {F1}
  → Return F1

Operation 2: create(And, a, b)
  → Found in unique_table_!
  → Return F1 (same pointer)

Operation 3: create(Or, F1, c)
  → Creates new Formula F2: (Or, F1, c)
  → F2 references F1 (structural sharing!)
  → unique_table_ = {F1, F2}
  → Return F2

Result:
  F1: And(a, b)  ←────┐
  F2: Or(F1, c)  ──────┘ (shares F1)

Memory: 2 formulas instead of 3 (if no sharing)
```

---

## Simplification Algorithm

### O(n) Simplify with HashSet

```
simplify_and(Formula* f):
    Input: And(l, r)
    Goal: Simplify to canonical form

    ┌─────────────────────────────────────┐
    │  Step 1: Recursively simplify      │
    │  children                           │
    │                                     │
    │  sl = simplify(l)                   │
    │  sr = simplify(r)                   │
    └─────────────────┬───────────────────┘
                      │
                      ▼
    ┌─────────────────────────────────────┐
    │  Step 2: Flatten AND chain         │
    │  into HashSet                       │
    │                                     │
    │  HashSet<Formula*> terms            │
    │  collect_and_terms(sl, terms)      │
    │  collect_and_terms(sr, terms)      │
    │                                     │
    │  Now: terms = {a, b, Not(c), d}    │
    └─────────────────┬───────────────────┘
                      │
                      ▼
    ┌─────────────────────────────────────┐
    │  Step 3: Remove conflicts          │
    │  (x & Not(x) → False)              │
    │                                     │
    │  for each term in terms:           │
    │    if Not(term) in terms:          │
    │      return False                  │
    └─────────────────┬───────────────────┘
                      │
                      ▼
    ┌─────────────────────────────────────┐
    │  Step 4: Remove True               │
    │  (x & True → x)                    │
    │                                     │
    │  terms.remove(True)                │
    └─────────────────┬───────────────────┘
                      │
                      ▼
    ┌─────────────────────────────────────┐
    │  Step 5: Check empty               │
    │  (empty set → True)                │
    │                                     │
    │  if terms.empty():                  │
    │    return True                     │
    └─────────────────┬───────────────────┘
                      │
                      ▼
    ┌─────────────────────────────────────┐
    │  Step 6: Rebuild AND chain         │
    │  from remaining terms              │
    │                                     │
    │  result = terms[0]                  │
    │  for i = 1 to terms.size()-1:      │
    │    result = create_and(result,     │
    │                       terms[i])    │
    │  return result                     │
    └─────────────────────────────────────┘
```

### Example Walkthrough

```
Input: ((a & True) & b) & (a & Not(c))

Step 1: Simplify children
  → All already simplified

Step 2: Flatten to HashSet
  terms = {a, True, b, a, Not(c)}
  → Dedup: {a, True, b, Not(c)}

Step 3: Check conflicts
  Is a in terms? Yes.
  Is Not(a) in terms? No.
  Is b in terms? Yes.
  Is Not(b) in terms? No.
  Is Not(c) in terms? Yes.
  Is c in terms? No.
  → No conflicts, continue

Step 4: Remove True
  terms.remove(True)
  → terms = {a, b, Not(c)}

Step 5: Check empty
  → Not empty, continue

Step 6: Rebuild AND chain
  result = a
  result = And(a, b)
  result = And(And(a, b), Not(c))

Output: And(And(a, b), Not(c))
Simplified from: And(And(And(a, True), b), And(a, Not(c)))
```

---

## Memory Management

### FormulaPool Lifecycle

```
┌─────────────────────────────────────────────────────────┐
│  Stage 1: Creation                                      │
│  ┌───────────────────────────────────────────────────┐ │
│  │ FormulaPool pool;                                  │ │
│  │                                                    │ │
│  │ - unique_table_: empty                             │ │
│  │ - formulas_: empty                                 │ │
│  │ - var_names_: empty                                │ │
│  └───────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  Stage 2: Variable Declaration                          │
│  ┌───────────────────────────────────────────────────┐ │
│  │ pool.declare_variables(outputs, inputs);          │ │
│  │                                                    │ │
│  │ - var_names_: ["s1", "s2", "p1", "p2"]           │ │
│  │ - var_ids_: {name → id mapping}                   │ │
│  │ - No Formula objects created yet                  │ │
│  └───────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  Stage 3: Formula Creation                              │
│  ┌───────────────────────────────────────────────────┐ │
│  │ Formula* f1 = pool.create_and(a, b);              │ │
│  │ Formula* f2 = pool.create_or(c, d);               │ │
│  │                                                    │ │
│  │ formulas_ = [unique_ptr<Formula>(f1),             │ │
│  │             unique_ptr<Formula>(f2)]              │ │
│  │ unique_table_ = {f1, f2}                           │ │
│  │                                                    │ │
│  │ Key: formulas_ vector OWNS all objects            │ │
│  └───────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  Stage 4: Transformation (Creates More Formulas)       │
│  ┌───────────────────────────────────────────────────┐ │
│  │ Formula* f3 = f1->simplify(pool);                  │ │
│  │                                                    │ │
│  │ formulas_ = [..., f1, f2,                         │ │
│  │             unique_ptr<Formula>(f3)]              │ │
│  │ unique_table_ = {f1, f2, f3}                       │ │
│  │                                                    │ │
│  │ Note: f1 and f2 still exist (referenced by DFA)  │ │
│  └───────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  Stage 5: Pool Destruction (RAII)                       │
│  ┌───────────────────────────────────────────────────┐ │
│  │ } // pool goes out of scope                        │ │
│  │                                                    │ │
│  │ ~FormulaPool():                                   │ │
│  │   1. unique_table_.clear()                        │ │
│  │   2. formulas_.clear()                            │ │
│  │      → All Formula objects destroyed              │ │
│  │   3. var_names_.clear()                           │ │
│  │   4. var_ids_.clear()                             │ │
│  │                                                    │ │
│  │ Result: Zero memory leaks                         │ │
│  └───────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

### Ownership Diagram

```
┌──────────────────────────────────────────────────────────┐
│                     FormulaPool                           │
│  ┌────────────────────────────────────────────────────┐ │
│  │ formulas_: vector<unique_ptr<Formula>>             │ │
│  │ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐     │ │
│  │ │   0  │ │   1  │ │   2  │ │   3  │ │   4  │ ... │ │
│  │ │  own │ │  own │ │  own │ │  own │ │  own │     │ │
│  │ └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘     │ │
│  │    │        │        │        │        │          │ │
│  └────┼────────┼────────┼────────┼────────┼──────────┘ │
│       │        │        │        │        │             │
│       ▼        ▼        ▼        ▼        ▼             │
│    ┌────┐  ┌────┐  ┌────┐  ┌────┐  ┌────┐              │
│    │ F1 │  │ F2 │  │ F3 │  │ F4 │  │ F5 │  ...         │
│    │And │  │ Or │  │Not │  │Next│  │Var │              │
│    │a,b │  │c,d │  │e   │  │f   │  │id=0│              │
│    └─┬──┘  └─┬──┘  └────┘  └────┘  └────┘              │
│      │        │                                              │
│      │ references (raw pointers)                           │
│      ▼                                                       ▼
│   ┌────┐                                                  ┌────┐
│   │DFA │                                                  │DFA │
│   │State│                                                 │State│
│   └────┘                                                  └────┘
│                                                           │
│   Key:                                                     │
│   - FormulaPool OWNS all formulas (unique_ptr)            │
│   - DFA States reference formulas (raw pointers)          │
│   - When FormulaPool destroyed, all formulas freed        │
│   - DFA States must be destroyed before FormulaPool       │
└───────────────────────────────────────────────────────────┘
```

---

## Summary of Key Flows

### Quick Reference

| Flow | Input | Output | Key Method |
|------|-------|--------|-------------|
| **Formula Creation** | OpType, children | Formula* | `FormulaPool::create()` |
| **Variable Declaration** | partition file or vectors | var_ids | `declare_variables()` |
| **Canonicalization** | Formula struct | unique Formula* | hash + lookup |
| **Simplification** | complex formula | simplified formula | `simplify()` with HashSet |
| **NNF Transform** | formula with Not everywhere | NNF formula | `nnf()` recursive |
| **XNF Transform** | NNF formula | XNF formula | `xnf_with_tail()` |
| **Formula Progression** | formula, edge, vars | next-state formula | `rmnext()` |

---

## Visualization Tips for Implementation

When implementing, use these diagrams to verify:

1. **Formula Creation**: Every `create()` call goes through canonicalization
2. **Variable Declaration**: Two-phase process (outputs then inputs)
3. **Transformations**: Always return NEW Formula*, never modify
4. **Memory**: FormulaPool owns everything, DFA states reference
5. **Simplification**: HashSet for O(n) performance
