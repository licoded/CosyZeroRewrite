# Simplification Algorithm Deep Analysis

**Date**: 2025-01-01
**Status**: In-Depth Analysis
**Related TODO**: TODO 3 from [TODOs.md](./TODOs.md)

---

## Executive Summary

Original implementation uses **iterative flattening + sort-based deduplication** for simplification.

**Key characteristics**:
- **split()**: O(n) iterative flattening of AND/OR chains
- **Deduplication**: O(n log n) via `std::sort()` and pointer comparison
- **Chain structure**: Right-leaning linked list (not balanced tree)
- **_simp cache**: Avoids recomputation
- **simplify_and_weak**: O(n) merge for already-simplified formulas

**Note**: `is_conflict()` function exists but is **disabled** (returns false). Original complex conflict detection code is commented out. If needed for correctness, can be re-enabled later.

---

## Part 1: Data Structure - Right-Leaning Chain

### 1.1 How AND/OR Formulas Are Stored

The original implementation uses **right-leaning chain structure** for AND/OR:

```
Input formula:  a & b & c & d

Memory structure:
      &
     / \
    a   &
       / \
      b   &
         / \
        c   d
```

This is NOT a balanced tree. It's a linked list using `left` (current) and `right` (next).

### 1.2 af_now() and af_next() - Chain Traversal

**Location**: `aalta_formula.cpp:1409-1427`

```cpp
// Return the "current" value in the chain
aalta_formula* af_now(int op) {
  if (_op == op)
    return _left;   // For AND/OR: left = current value
  return unique();
}

// Return the "next" node in the chain
aalta_formula* af_next(int op) const {
  if (_op == op)
    return _right;  // For AND/OR: right = rest of chain
  return NULL;
}
```

**Visualizing the traversal**:

```
Chain: a & b & c & d
Stored as: (& a (& b (& c d)))

Traversal using af_now() / af_next():

Step 1: current = root
        af_now(And)  → a  (current value)
        af_next(And) → (& b (& c d))  (rest of chain)

Step 2: current = af_next(result)
        af_now(And)  → b
        af_next(And) → (& c d)

Step 3: current = af_next(result)
        af_now(And)  → c
        af_next(And) → d

Step 4: current = af_next(result)
        af_now(And)  → d  (not AND, so returns itself)
        af_next(And) → NULL  (end of chain)
```

---

## Part 2: The split() Function - Iterative Flattening

### 2.1 Function Signature

```cpp
void split(int op, std::list<aalta_formula*>& af_list, bool to_simplify)
```

### 2.2 Algorithm Visualization

**Input**: `((a & b) & c) & d`  (already a right-leaning chain)

**Trace through the algorithm**:

```
Initial State:
  af = ((a & b) & c) & d
  store = []
  af_list = []

────────────────────────────────────────

Iteration 1: af->_op == And
  store.push_back(d)     → store = [d]
  af = (a & b) & c

  Structure visualization:
           [&]                    [&]                  [&]
          /  \                  /  \                 /  \
         a    [&]       →     a     c      →       a     NULL
              /  \
             b    c
           (left) (right→stored)

────────────────────────────────────────

Iteration 2: af->_op == And
  store.push_back(c)     → store = [d, c]
  af = a & b

────────────────────────────────────────

Iteration 3: af->_op == And
  store.push_back(b)     → store = [d, c, b]
  af = a

────────────────────────────────────────

Iteration 4: af->_op != And (it's 'a', an atom)
  af_list.push_back(a)   → af_list = [a]
  af = store.front() = b
  store.pop_front()      → store = [d, c]

────────────────────────────────────────

Iteration 5: af->_op != And (it's 'b')
  af_list.push_back(b)   → af_list = [a, b]
  af = store.front() = c
  store.pop_front()      → store = [d]

────────────────────────────────────────

Iteration 6: af->_op != And (it's 'c')
  af_list.push_back(c)   → af_list = [a, b, c]
  af = store.front() = d
  store.pop_front()      → store = []

────────────────────────────────────────

Iteration 7: af->_op != And (it's 'd')
  af_list.push_back(d)   → af_list = [a, b, c, d]
  store is empty → BREAK

Final Output: [a, b, c, d] - flattened list!
```

### 2.3 Flow Chart

```
┌─────────────────────────────────────────────────────────────────┐
│                     split(op, af_list)                          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │  af != NULL ?   │
                    └────────┬────────┘
                             │
                ┌────────────┴────────────┐
                │ Yes                      │ No
                ▼                          ▼
        ┌───────────────┐           ┌──────────────┐
        │ af->_op == op?│           │    RETURN     │
        └───────┬───────┘           └──────────────┘
                │
       ┌────────┴────────┐
       │ Yes              │ No
       ▼                  ▼
┌──────────────┐   ┌──────────────┐
│ store.push   │   │ af_list.push │
│ (af->_right) │   │ (af)         │
│ af = af->_left│   │ af = store   │
└──────────────┘   │ .front()     │
                   │ store.pop    │
                   └───────┬──────┘
                           │
                           ▼
                   ┌──────────────────┐
                   │ Repeat from top │
                   └──────────────────┘
```

---

## Part 3: simplify_and() - Complete Walkthrough

### 3.1 Function Overview

```cpp
aalta_formula* simplify_and(aalta_formula *l, aalta_formula *r)
```

**Purpose**: Simplify `l & r` where l and r are **possibly unsimplified**.

### 3.2 Complete Flow Chart

```
┌─────────────────────────────────────────────────────────────────┐
│                  simplify_and(l, r)                             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                ┌─────────────────────────┐
                │ l->_simp && r->_simp ?   │
                └──────────┬──────────────┘
                           │
              ┌────────────┴────────────┐
              │ Yes                      │ No
              ▼                          ▼
    ┌──────────────────┐      ┌──────────────────────┐
    │simplify_and_weak│      │  l->split(And, list) │
    │  (fast path)     │      │  r->split(And, list) │
    └──────────────────┘      └───────────┬──────────┘
                                       │
                                       ▼
                              ┌──────────────────┐
                              │ Create array[]   │
                              │ afp[n]            │
                              └───────────┬──────┘
                                          │
                                          ▼
                              ┌──────────────────────┐
                              │ Scan af_list, build  │
                              │ afp array, skip True │
                              └───────────┬──────────┘
                                          │
                                          ▼
                              ┌──────────────────────┐
                              │   Found False?       │──Yes→ return FALSE
                              └───────────┬──────────┘
                                          │ No
                                          ▼
                              ┌──────────────────────┐
                              │ n == 0 ?             │──Yes→ return TRUE
                              └───────────┬──────────┘
                                          │ No
                                          ▼
                              ┌──────────────────────┐
                              │   sort(afp, afp+n)   │
                              └───────────┬──────────┘
                                          │
                                          ▼
                              ┌──────────────────────┐
                              │ Deduplicate in-place │
                              │ (compare pointers)   │
                              └───────────┬──────────┘
                                          │
                                          ▼
                              ┌──────────────────────┐
                              │ n == 1 ?             │──Yes→ return afp[0]
                              └───────────┬──────────┘
                                          │ No
                                          ▼
                              ┌──────────────────────┐
                              │ Rebuild AND chain    │
                              │ Set _simp = self     │
                              └───────────┬──────────┘
                                          │
                                          ▼
                              ┌──────────────────────┐
                              │  return ret->unique()│
                              └──────────────────────┘
```

### 3.3 Detailed Example Trace

**Input**: `(a & b) & (a & c)`

**Step 1: Split**
```
l = a & b  →  l.split(And, af_list, true)
r = a & c  →  r.split(And, af_list, true)

af_list = [a, b, a, c]
```

**Step 2: Build array, filter True**
```
afp = [a, b, a, c]
n = 4
```

**Step 3: Sort**
```
After sort(afp, afp+n):
afp = [a, a, b, c]  (sorted by pointer value!)
```

**Step 4: Deduplicate**
```
for (i=0, j=1; j<n; ++j)
  if (afp[i] != afp[j])
    afp[++i] = afp[j];

i=0: j=1, afp[0]=a, afp[1]=a, equal, skip
i=0: j=2, afp[0]=a, afp[2]=b, not equal
     → afp[1] = afp[2] = b, i=1
i=1: j=3, afp[1]=b, afp[3]=c, not equal
     → afp[2] = afp[3] = c, i=2

n = i+1 = 3
afp = [a, b, c, ...]
```

**Step 5: Rebuild chain**
```
n = 3
l = afp[0] = a
r = afp[--n] = afp[2] = c

while (--n)  // n becomes 2, then 1, loop runs once
{
  r = And(afp[1], r).unique()  // r = And(b, c)
  r->_simp = r
}

ret = And(l, r).unique()  // ret = And(a, And(b, c))
ret->_simp = ret
```

**Output**: `a & b & c` (with duplicate `a` removed!)

---

## Part 4: simplify_and_weak() - The Merge Algorithm

### 4.1 When Is It Called?

```cpp
if (l->_simp != NULL && r->_simp != NULL)
  return simplify_and_weak(l, r);
```

Both inputs are **already simplified**. This means:
- They are already in right-leaning chain form
- They are already sorted by pointer value
- No True values (they were removed during simplification)
- No duplicates

### 4.2 核心逻辑：拆分 → 去重 → 合并

```
┌─────────────────────────────────────────────────────────────────┐
│              simplify_and_weak(l, r)                              │
│        本质：拆分 l 和 r，去重后用 AND 合并                      │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
              ┌─────────────────────────────────────┐
              │ Step 1: 拆分 (split)               │
              │                                    │
              │   l_chain → [a, c, e]              │
              │   r_chain → [b, c, d]              │
              └──────────────────┬──────────────────┘
                                 │
                                 ▼
              ┌─────────────────────────────────────┐
              │ Step 2: 合并去重 (merge)           │
              │                                    │
              │   a < b → take a                   │
              │   c > b → take b                   │
              │   c == c → take one (去重!)        │
              │   d < e → take d                   │
              │   e remaining → take e             │
              │                                    │
              │   Result: [a, b, c, d, e]          │
              └──────────────────┬──────────────────┘
                                 │
                                 ▼
              ┌─────────────────────────────────────┐
              │ Step 3: 重建 AND 链               │
              │                                    │
              │   a & b & c & d & e                  │
              │   (右倾链结构)                      │
              └─────────────────────────────────────┘
```

**为什么高效？**
- 输入已排序、已去重 → O(n) 归并算法
- 不需要调用 split() 展开链（直接用 af_now/af_next 遍历）
- 不需要排序（输入已排序）

---

## Part 5: simplify_or() - The OR Version

### 5.1 Key Differences from simplify_and

| Aspect | simplify_and | simplify_or |
|--------|--------------|-------------|
| **False/True** | False short-circuits | True short-circuits |
| **Identity** | True (empty AND) | False (empty OR) |
| **Conflict** | a & !a → False | a \| !a → True |
| **Until opt.** | No | Yes (mutex rules) |

### 5.2 Simplify Rules for OR

```cpp
// Basic rules
False | a  →  a           // Remove False
True | a   →  True        // Short circuit
a | a     →  a           // Deduplicate
a | !a    →  True        // Mutex (covered by mutex())

// Until-specific optimizations
a | (b U (!a | ...))  → True
(a | ...) U a         → a | ...
```

### 5.3 The mutex() Function

```cpp
// Check if formula contains a and !a (atomic only!)
bool mutex(int op, int_set& pos, int_set& neg) {
  // Collect all atomic propositions in pos
  // Collect all negated atomics in neg
  // Check if any atom appears in both pos and neg
}
```

**Note**: `mutex()` only works for **atomic propositions**, not complex formulas!

---

## Part 6: Complexity Analysis (Revised)

### 6.1 Actual Complexities

| Function | Original Claim | Actual Complexity | Notes |
|----------|----------------|-------------------|-------|
| **split()** | O(n) | O(n) | Iterative, single pass |
| **simplify_and_weak** | O(n) | O(n) | Merge of sorted lists |
| **simplify_and** | O(n²) claimed | **O(n log n)** | Sort dominates, conflict disabled |
| **simplify_or** | O(n²) claimed | **O(n log n)** | Sort dominates |

### 6.2 Why NOT O(n²)?

The nested loop conflict check in simplify_and appears to be O(n²), but `is_conflict()` is **disabled** (always returns false), so this loop is effectively a no-op.

**Actual complexity**: `O(n log n)` dominated by `std::sort()`.

---

## Part 7: The _simp Cache

### 7.1 Purpose

The `_simp` pointer caches the simplified version of a formula:

```cpp
aalta_formula* _simp;  // Points to simplified version
```

### 7.2 How It Works

```
First call: f->simplify()
  → Compute simplified version
  → Store in f->_simp
  → Return f->_simp

Subsequent calls: f->simplify()
  → if (_simp != NULL) return _simp;  ← Cached!
```

### 7.3 Why simplify_and_weak Exists

```
simplify_and(a, b)
  → if (a->_simp && b->_simp)
       return simplify_and_weak(a, b)  ← Fast path!

simplify_and_weak assumes:
  - Both inputs are already simplified
  - Both are sorted chains
  - Can use merge algorithm (O(n))
```

---

## Part 8: Visual Summary

### 8.1 The Complete Simplification Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│    Original Formula:  ((a & b) & (c & !a)) & d                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Step 1: split()                           │
│                                                                 │
│  l.split(And, ...)  →  [a, b, c, !a]                            │
│  r.split(And, ...)  →  [d]                                      │
│                                                                 │
│  Combined: [a, b, c, !a, d]                                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Step 2: Filter                               │
│                                                                 │
│  - Remove True (none in this case)                              │
│  - Check for False (none in this case)                          │
│                                                                 │
│  Result: [a, b, c, !a, d]                                       │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Step 3: Sort                                │
│                                                                 │
│  Before: [a, b, c, !a, d]  (arbitrary order)                    │
│  After:  [!a, a, b, c, d]  (sorted by pointer)                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Step 4: Deduplicate                            │
│                                                                 │
│  Compare adjacent elements:                                     │
│  !a != a → keep both                                            │
│  a != b → keep both                                             │
│  b != c → keep both                                             │
│  c != d → keep both                                             │
│                                                                 │
│  Result: [!a, a, b, c, d]  (no duplicates to remove)            │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Step 5: Rebuild Chain                         │
│                                                                 │
│  Build right-leaning chain:                                     │
│                                                                 │
│      &                   &                                     │
│     / \                 / \                                    │
│    !a   &       →      a   &                                   │
│        / \                 / \                                  │
│       a   &               b   &                                 │
│          / \                   / \                                │
│         b   &               c   d                                │
│            / \                                                   │
│           c   d                                                │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              Result: !a & a & b & c & d                           │
│                                                                 │
│  Note: Original is_conflict() function is disabled, so          │
│        a & !a conflicts are NOT detected!                       │
└─────────────────────────────────────────────────────────────────┘
```

---

## Part 9: simplify_until() and simplify_release()

### 9.1 simplify_until() - Rule-Based Simplification

```cpp
aalta_formula* simplify_until(aalta_formula *l, aalta_formula *r)
```

**Rules in order**:

| Rule | Formula | Result | Explanation |
|------|---------|--------|-------------|
| 1 | `False U a` | `a` | False until a is just a |
| 2 | `a U False` | `False` | a until False is False |
| 3 | `a U True` | `True` | a until True is True |
| 4 | `a U (a \| ...)` | `a \| ...` | Right absorption |
| 5 | `a U (a U b)` | `a U b` | Left Until absorption |
| 6 | `a U (b U a)` | `b U a` | Right Until absorption |
| 7 | `X a U a` | `X a \| a` | Next distribution |
| 8 | `X a U X b` | `X(a U b)` | Next extraction |

### 9.2 simplify_release() - Rule-Based Simplification

```cpp
aalta_formula* simplify_release(aalta_formula *l, aalta_formula *r)
```

**Rules in order**:

| Rule | Formula | Result | Explanation |
|------|---------|--------|-------------|
| 1 | `True R a` | `a` | True release a is just a |
| 2 | `a R False` | `False` | a release False is False |
| 3 | `a R True` | `True` | a release True is True |
| 4 | `a R (a & ...)` | `a & ...` | Right absorption |
| 5 | `(a \| ...) R a` | `a` | Left absorption |
| 6 | `!a R a` | `False R a` | Not absorption |

---

## Part 10: Simplify_next() - Trivial Case

```cpp
aalta_formula* simplify_next(aalta_formula *af) {
  aalta_formula *s = af->simplify();

  switch (s->_op) {
    case False:
      return FALSE;  // X False = False
    // case True:
    //   return TRUE;  // X True = True (commented out!)
    default:
      return Next(af).unique();
  }
}
```

**Note**: `X True = True` optimization is **disabled** in original code!

---

## Part 11: Summary and Recommendations

### 11.1 Key Findings

1. **Actual complexity is O(n log n)** - Dominated by sorting, not O(n²) as originally thought
2. **Chain structure** - Right-leaning linked list (not balanced tree)
3. **af_now/af_next** - Chain traversal functions
4. **_simp cache** - Avoids re-simplification
5. **simplify_and_weak** - Optimization for already-simplified formulas

**Important Note**: `is_conflict()` is **disabled** in the original implementation. This means conflicts like `a & !a` are NOT detected during simplification. The function exists but always returns false.

### 11.2 Recommendations for New Design

| Aspect | Original | New Design Recommendation |
|--------|----------|---------------------------|
| **Flattening** | split() iterative | Keep same approach |
| **Deduplication** | Sort + compare | Use HashSet (O(n)) |
| **Chain structure** | Right-leaning | Keep same structure |
| **_simp cache** | Pointer cache | Keep caching |

**Note on Conflict Detection**: Original implementation has `is_conflict()` disabled. Consider whether to implement conflict detection (e.g., `a & !a → False`) in the new design.

### 11.3 Proposed O(n) Algorithm

```cpp
Formula* simplify_and(Formula* left, Formula* right) {
  std::unordered_set<Formula*> terms;

  // Collect terms (O(n))
  collect_and_terms(left, terms);
  collect_and_terms(right, terms);

  // Check for conflicts during collection (O(n))
  if (has_conflict) {
    return create_false();
  }

  // Rebuild chain (O(n))
  return rebuild_and_chain(terms);
}

void collect_and_terms(Formula* f, unordered_set<Formula*>& terms) {
  if (f->op() == And) {
    collect_and_terms(f->left(), terms);
    collect_and_terms(f->right(), terms);
  } else if (f->op() == True) {
    // Skip True
  } else if (f->op() == False) {
    has_conflict = true;  // False in AND → False
  } else {
    // Check for a & !a
    if (f->op() == Not) {
      if (terms.find(f->right()) != terms.end()) {
        has_conflict = true;  // Found a and !a
        return;
      }
    }
    // Check if !a exists
    Formula* neg_f = create_not(f);
    if (terms.find(neg_f) != terms.end()) {
      has_conflict = true;
      return;
    }
    terms.insert(f);
  }
}
```

### 11.4 Open Questions

1. **Should we enable conflict detection?**
   - Original: Disabled (always returns false)
   - Pros: Can detect `a & !a` → False
   - Cons: Adds complexity

2. **Should we enable `X True = True` optimization?**
   - Original: Disabled
   - Simple optimization, no reason to disable

3. **Should we keep simplify_and_weak?**
   - Original: Optimization for already-simplified formulas
   - New: Could unify into single function with flag

---

## Part 12: Known Issues / Bug Tracking

### 12.1 Disabled `is_conflict()` - Potential Bug Source

**Status**: ⚠️ **KNOWN ISSUE** - Documented for root cause analysis

#### 12.1.1 Problem Description

The `is_conflict()` function in `aalta_formula.cpp:134` is **completely disabled**:

```cpp
bool aalta_formula::is_conflict(aalta_formula *af1, aalta_formula *af2) {
  return false;  // 直接返回 false！
  //@ TODO: 好想重构化简代码啊啊啊啊啊，可是好难重构啊啊啊啊啊啊   %>_<%
  // ... 后面所有代码都是死代码
}
```

#### 12.1.2 Impact

| Conflict Type | Detection | Result |
|--------------|-----------|--------|
| `a \| !a → True` | ✅ Works | `simplify_or` uses `mutex()` function |
| `a & !a → False` | ❌ **Broken** | `simplify_and` calls disabled `is_conflict()` |

**Consequence**: Formulas containing `a & !a` are **NOT simplified to `False`** during simplification.

#### 12.1.3 Why This Might Not Break Everything

The original implementation still works because:

1. **DFA construction may catch conflicts later**: The progression algorithm (`FormulaProgression` in `af_utils.cpp`) evaluates formulas on specific edges, and unsatisfiable formulas will naturally result in no valid transitions.

2. **BDD-based operations may implicitly handle contradictions**: When building BDDs for edge constraints, `a & !a` evaluates to `False` at the BDD level.

3. **SAT/SMT solvers may detect unsatisfiability**: If the formula is passed to Z3 or other solvers, they will detect contradictions.

#### 12.1.4 Bug Symptoms to Watch For

If you encounter any of these issues, the disabled `is_conflict()` may be the root cause:

1. **Unexpectedly large DFAs**: Formulas that should simplify to `False` remain complex, leading to larger automata.

2. **Performance issues**: Unnecessary state space exploration due to undetected contradictions.

3. **Incorrect synthesis results**: In rare cases, the algorithm might find strategies for unsatisfiable formulas (though this is unlikely if later stages handle it correctly).

4. **Test failures**: Formulas like `a & !a` not evaluating to `False` as expected.

#### 12.1.5 Code Locations

| File | Line | Function | Status |
|------|------|----------|--------|
| `aalta_formula.cpp` | 134 | `is_conflict()` | **Disabled** |
| `aalta_formula.cpp` | 444 | `simplify_and` → `is_conflict()` | Call is no-op |
| `aalta_formula.cpp` | 104 | `mutex()` | **Active** (for OR) |
| `aalta_formula.cpp` | 333, 340 | `simplify_or` → `mutex()` | Works correctly |
| `af_utils.cpp` | 249 | `check_conflict()` | **Active** (edge sets) |

#### 12.1.6 Resolution Options

| Option | Description | Effort | Impact |
|--------|-------------|--------|--------|
| **A. Leave as-is** | Document and rely on downstream handling | None | Low (system works) |
| **B. Enable `is_conflict()`** | Implement working conflict detection | Medium | Medium (may improve performance) |
| **C. Add simple check** | Just detect `a & !a` in simplify_and | Low | Medium (handles common case) |

#### 12.1.7 Recommendation for New Design

**Implement basic conflict detection** for the new Formula module:

```cpp
// Simple conflict detection for AND
bool has_conflict(Formula* f1, Formula* f2) {
  // Check for literal contradictions: a & !a
  if (f1->op() == Not && f1->right()->is_literal() &&
      f2->is_literal() && f1->right()->var_id() == f2->var_id())
    return true;
  if (f2->op() == Not && f2->right()->is_literal() &&
      f1->is_literal() && f2->right()->var_id() == f1->var_id())
    return true;
  return false;
}
```

This handles the most common case (`a & !a`) with minimal complexity.

---

## References

- **simplify_and**: `aalta_formula.cpp:401-467`
- **simplify_and_weak**: `aalta_formula.cpp:476-548`
- **simplify_or**: `aalta_formula.cpp:306-392`
- **simplify_next**: `aalta_formula.cpp:280-297`
- **simplify_until**: `aalta_formula.cpp:619-652`
- **simplify_release**: `aalta_formula.cpp:661-692`
- **is_conflict** (disabled): `aalta_formula.cpp:134-275`
- **split**: `aalta_formula.cpp:1254-1276`
- **af_now**: `aalta_formula.cpp:1409-1414`
- **af_next**: `aalta_formula.cpp:1422-1427`

---

**Document prepared by**: Claude Code in-depth analysis
**Date**: 2025-01-01
**Version**: 4.0 (Part 12: Known Issues / Bug Tracking added)

**Status**: ✅ Ready for user review
