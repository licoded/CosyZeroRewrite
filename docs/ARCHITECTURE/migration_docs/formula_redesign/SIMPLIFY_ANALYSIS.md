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
Formula* simplify_until(FormulaPool& pool, Formula* left, Formula* right)
```

**符号说明**: `X[!]` 表示 strong next (强下一步操作符)，要求状态必须转移。

**Rules in order**:

| Rule | Formula | Result | Explanation |
|------|---------|--------|-------------|
| 1 | `False U a` | `a` | False until a is just a |
| 2 | `a U False` | `False` | a until False is False |
| 3 | `a U True` | `True` | a until True is True |
| 4 | `a U (a \| ...)` | `a \| ...` | Right absorption |
| 5 | `a U (a U b)` | `a U b` | Left Until absorption |
| 6 | `a U (b U a)` | `b U a` | Right Until absorption |
| 7 | `(X[!] a) U a` | `a \| X[!] a` | Next distribution |
| 8 | `(X[!] a) U (X[!] b)` | `X[!](a U b)` | Next extraction |

### 9.2 simplify_release() - Rule-Based Simplification

```cpp
Formula* simplify_release(FormulaPool& pool, Formula* left, Formula* right)
```

**Rules in order**:

| Rule | Formula | Result | Explanation |
|------|---------|--------|-------------|
| 1 | `True R a` | `a` | True release a is just a |
| 2 | `a R False` | `False` | a release False is False |
| 3 | `a R True` | `True` | a release True is True |
| 4 | `a R (a & ...)` | `a & ...` | Right absorption |
| 5 | `(a \| ...) R a` | `a` | Left absorption |
| 6 | `(!a) R a` | `False R a` | Not absorption |

---

## Part 10: simplify_next() - Trivial Case

```cpp
Formula* simplify_next(FormulaPool& pool, Formula* operand)
```

**符号说明**: `X[!]` 表示 strong next (强下一步操作符)。

**Rules**:
| Rule | Formula | Result | Explanation |
|------|---------|--------|-------------|
| 1 | `X[!] False` | `False` | Next of false is false |
| 2 | `X[!] True` | `True` | Next of true is true (原实现中被禁用，新实现保持禁用) |

**Note**: `X[!] True = True` 优化在原 aalta 实现中被禁用，新实现保持该行为。

---

## Part 11: Summary and Recommendations

### 11.1 Key Findings - Original (aalta) Implementation

1. **Actual complexity is O(n log n)** - Dominated by sorting, not O(n²) as originally thought
2. **Chain structure** - Right-leaning linked list (not balanced tree)
3. **af_now/af_next** - Chain traversal functions
4. **_simp cache** - Avoids re-simplification
5. **simplify_and_weak** - Optimization for already-simplified formulas

**Important Note**: `is_conflict()` is **disabled** in the original implementation. This means conflicts like `a & !a` are NOT detected during simplification. The function exists but always returns false.

### 11.2 New Implementation Status (2026-01)

**已实现的改进**:

| Aspect | Original | New Implementation | Status |
|--------|----------|-------------------|--------|
| **Flattening** | split() iterative | `collect_binary_terms()` 递归展开 | ✅ |
| **Deduplication** | Sort + compare (O(n log n)) | HashSet + hash consing (O(n)) | ✅ |
| **Conflict detection** | Disabled | `has_complementary_literals()` 已启用 | ✅ |
| **Chain structure** | Right-leaning | Right-leaning | ✅ |
| **Simp cache** | Per-object `_simp` pointer | N/A (immutable design) | ✅ |

**冲突检测**:
- ✅ 新实现已启用 `has_complementary_literals()` 函数
- ✅ 检测 `a & !a → False` (AND 中的互补字面量)
- ✅ 检测 `a | !a → True` (OR 中的互补字面量)
- ⚠️ 仅检测 NNF 形式的字面量，不检测复杂子公式

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

### 11.4 Open Questions (已解决)

| 问题 | 原实现状态 | 新实现状态 |
|------|-----------|-----------|
| **启用冲突检测?** | Disabled (always returns false) | ✅ **已启用** `has_complementary_literals()` |
| **启用 `X[!] True = True`?** | Disabled | ❌ 保持禁用 (与原实现一致) |
| **保留 simplify_and_weak?** | Optimization | ❌ 不需要 (HashSet 自动去重) |

---

## Part 12: Known Issues / Bug Tracking

### 12.1 Original (aalta) Implementation - Disabled `is_conflict()`

**Status**: ✅ **FIXED in new implementation** - 此问题仅存在于原 aalta 实现

#### 12.1.1 Problem Description (Original)

The `is_conflict()` function in `aalta_formula.cpp:134` is **completely disabled**:

```cpp
bool aalta_formula::is_conflict(aalta_formula *af1, aalta_formula *af2) {
  return false;  // 直接返回 false！
  // ...
}
```

#### 12.1.2 Impact

| Conflict Type | Original Detection | New Implementation |
|--------------|-------------------|-------------------|
| `a \| !a → True` | ✅ Works (`mutex()`) | ✅ Works (`has_complementary_literals()`) |
| `a & !a → False` | ❌ **Broken** | ✅ **Fixed** |

#### 12.1.3 Resolution in New Implementation

**Location**: `src/formula/simplify.cpp:49-74`

```cpp
bool has_complementary_literals(const std::unordered_set<Formula*>& terms,
                                 FormulaPool& pool) {
    std::unordered_set<Formula*> positives;  // v0, v1, ...
    std::unordered_set<Formula*> negatives;  // !v0, !v1, ...

    for (Formula* f : terms) {
        if (f->is_not()) {
            if (positives.find(f->left()) != positives.end()) {
                return true;  // a and !a both present
            }
            negatives.insert(f);
        } else if (f->is_literal()) {
            Formula* negated = pool.create_not(f);
            if (negatives.find(negated) != negatives.end()) {
                return true;  // !a and a both present
            }
            positives.insert(f);
        }
    }
    return false;
}
```

**✅ 已实现**:
- AND 冲突检测: `a & !a → False` (simplify_and:379-382)
- OR 重言式检测: `a | !a → True` (simplify_or:440-443)

---

## References

### Original (aalta) Implementation
- **simplify_and**: `aalta_formula.cpp:401-467`
- **simplify_and_weak**: `aalta_formula.cpp:476-548`
- **simplify_or**: `aalta_formula.cpp:306-392`
- **simplify_next**: `aalta_formula.cpp:280-297`
- **simplify_until**: `aalta_formula.cpp:619-652`
- **simplify_release**: `aalta_formula.cpp:661-692`
- **is_conflict** (disabled): `aalta_formula.cpp:134-275`
- **split**: `aalta_formula.cpp:1254-1276`

### New Implementation
- **simplify.cpp**: `src/formula/simplify.cpp`
- **collect_binary_terms()**: `simplify.cpp:22-34`
- **has_complementary_literals()**: `simplify.cpp:49-74`
- **simplify_and()**: `simplify.cpp:345-386`
- **simplify_or()**: `simplify.cpp:406-447`
- **simplify_until()**: `simplify.cpp:161-208`
- **simplify_release()**: `simplify.cpp:226-263`
- **simplify_next()**: `simplify.cpp:276-289`

---

**Document prepared by**: Claude Code in-depth analysis
**Date**: 2025-01-01
**Version**: 4.0 (Part 12: Known Issues / Bug Tracking added)

**Status**: ✅ Ready for user review
