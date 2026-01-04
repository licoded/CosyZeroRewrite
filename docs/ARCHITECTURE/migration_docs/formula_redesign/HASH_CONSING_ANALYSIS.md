# Hash Consing Efficiency Analysis

**Date**: 2025-01-01
**Status**: Analysis Complete
**Related TODO**: TODO 2 from [TODOs.md](./TODOs.md)

---

## Executive Summary

The original `aalta_formula` implementation uses **hash consing** with **hash + structural comparison** for deduplication. The implementation is efficient but has some characteristics that impact the new design:

**Key Findings**:
- Uses **cached hash values** (computed once, stored)
- Uses **hash-based lookup** with **structural equality verification**
- Uses **pointer equality** for child comparisons (exploits canonicalization)
- Global `all_afs` set grows during DFA construction, cleared per top-level formula
- **Deduplication ratio** is typically high for LTLf formulas due to repeated subformulas

**Recommendation for New Design**: Keep hash+structural comparison, use cached hash, and adopt `std::unordered_set` for compatibility.

---

## 1. Original Implementation Architecture

### 1.1 Data Structures

```cpp
// Global set of all unique formulas (cleared per top-level formula)
static afp_set all_afs;

// Type definition (aalta_formula.h:133)
typedef hash_set<aalta_formula *, af_prt_hash2, af_prt_eq> afp_set;

// hash_set is std::tr1::unordered_set or std::unordered_set
```

### 1.2 Hash Function (`af_prt_hash2`)

```cpp
struct af_prt_hash2
{
  size_t operator() (const aalta_formula *af_prt) const
  {
    return af_prt->_hash;  // Return cached hash value
  }
};
```

### 1.3 Equality Function (`af_prt_eq`)

```cpp
struct af_prt_eq
{
  bool operator() (const aalta_formula *af_prt1, const aalta_formula *af_prt2) const
  {
    return *af_prt1 == *af_prt2;  // Use operator== for structural comparison
  }
};
```

### 1.4 Structural Equality (`operator==`)

```cpp
bool aalta_formula::operator == (const aalta_formula& af) const
{
  return _op == af._op &&
         _left == af._left &&   // Pointer comparison (safe due to canonicalization)
         _right == af._right && // Pointer comparison (safe due to canonicalization)
         _tag == af._tag;
}
```

---

## 2. Hash Computation (`clc_hash`)

**Location**: `aalta_formula.cpp:726-740`

```cpp
inline void aalta_formula::clc_hash()
{
  _hash = HASH_INIT;  // 1315423911
  _hash = (_hash << 5) ^ (_hash >> 27) ^ _op;

  if (_left != NULL)
    _hash = (_hash << 5) ^ (_hash >> 27) ^ _left->_hash;
  if (_right != NULL)
    _hash = (_hash << 5) ^ (_hash >> 27) ^ _right->_hash;

  _hash = (_hash << 5) ^ (_hash >> 27) ^ (size_t)_tag;
}
```

### 2.1 Hash Function Characteristics

| Property | Value | Notes |
|----------|-------|-------|
| **Type** | Rolling hash | Each component mixed with shift/XOR |
| **Initial value** | 1315423911 | Magic constant (HASH_INIT) |
| **Mixing operation** | `(h << 5) ^ (h >> 27)` | Bit rotation-like operation |
| **Components** | `_op`, `_left->_hash`, `_right->_hash`, `_tag` | Structure + content |
| **Caching** | Yes | Computed once, stored in `_hash` |
| **Collision resistance** | Moderate | XOR-based mixing can have collisions |

### 2.2 Hash Computation Cost

- **O(1)** for cached hash (just return `_hash`)
- **O(n)** for initial computation (recursive over formula tree)
- **Amortized O(1)** due to caching and subformula sharing

---

## 3. Hash Consing Flow (`unique()`)

**Location**: `aalta_formula.cpp:1518-1545`

```cpp
aalta_formula* aalta_formula::unique()
{
  if (_unique == NULL)
  {
    // Step 1: Hash-based lookup
    afp_set::const_iterator iter = all_afs.find(this);

    if (iter != all_afs.end())
    {
      // Step 2a: Found existing formula - reuse it
      _unique = (*iter);
    }
    else
    {
      // Step 2b: Not found - create new unique formula
      _unique = clone();
      _unique->_id = _max_id++;
      all_afs.insert(_unique);
      _unique->_unique = _unique;  // Self-reference
    }
  }

  if (_unique->_simp == NULL)
    _unique->_simp = _simp;

  return _unique;
}
```

### 3.1 Lookup Process

```
┌─────────────────────────────────────────────────────────────────┐
│  Create formula: aalta_formula(And, a, b)                       │
├─────────────────────────────────────────────────────────────────┤
│  1. Constructor computes hash: clc_hash()                        │
│     → _hash = H(And, hash(a), hash(b), tag)                     │
│                                                                 │
│  2. Call unique()                                                │
│     → all_afs.find(this)  [hash-based lookup]                   │
│                                                                 │
│  3a. FOUND:                                                     │
│      → Return existing pointer (no allocation)                  │
│                                                                 │
│  3b. NOT FOUND:                                                 │
│      → clone() the formula                                      │
│      → Assign unique ID                                         │
│      → Insert into all_afs                                      │
│      → Return new pointer                                       │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 Hash Table Lookup Complexity

| Operation | Complexity | Notes |
|-----------|------------|-------|
| **Hash computation** | O(1) cached | Uses pre-computed `_hash` |
| **Bucket lookup** | O(1) average | Unordered_set uses buckets |
| **Equality check** | O(1) | Pointer comparison for children |
| **Overall** | O(1) average | Standard hash table performance |

---

## 4. Structural Equality Analysis

### 4.1 Why Pointer Comparison Works

```cpp
return _left == af._left && _right == af._right;
```

This works because of **canonicalization**:
- When a formula is created, its children are already `unique()`'d
- Therefore, equal subformulas have the **same pointer**
- Pointer equality (`==`) suffices for structural equality

### 4.2 Recursive vs Non-Recursive Comparison

**Original implementation**: Non-recursive (pointer comparison)
```
Compare(And(a, b), And(a', b'))
  → Compare(op, op)           : value compare
  → Compare(left, left')      : pointer compare (O(1))
  → Compare(right, right')    : pointer compare (O(1))
  → Compare(tag, tag')        : pointer compare
```

**Without canonicalization**: Would need recursive compare
```
Compare(And(a, b), And(a', b'))
  → Compare(op, op)           : value compare
  → Compare(left, left')      : recursive (O(n))
  → Compare(right, right')    : recursive (O(n))
  → Compare(tag, tag')        : pointer compare
```

**Key insight**: The original design exploits canonicalization for O(1) equality checks.

---

## 5. Performance Characteristics

### 5.1 Hash Consing Operations

| Operation | Cost | Frequency |
|-----------|------|-----------|
| **Hash computation** | O(n) first time, O(1) cached | Once per formula |
| **Hash lookup** | O(1) average | Every `unique()` call |
| **Equality check** | O(1) pointer compare | On hash collision |
| **Insertion** | O(1) average + allocation | Only for new formulas |

### 5.2 Deduplication Ratio

**Typical LTLf formulas** have high deduplication due to:
- Repeated atomic propositions
- Common subformulas (e.g., `X a`, `!a`)
- Formula transformations (NNF, XNF) create repeated patterns

**Example**:
```
Formula: (a U b) & X(a U b) & XX(a U b)

Without hash consing: 3 distinct formulas for (a U b)
With hash consing:    1 formula shared 3 times
```

### 5.3 Memory Impact

| Metric | Impact |
|--------|--------|
| **Memory per formula** | +8 bytes for `_hash`, +8 bytes for `_unique` pointer |
| **Global set overhead** | ~O(unique formulas) for hash table buckets |
| **Savings from sharing** | Potentially 50-90% reduction in total formula objects |

---

## 6. all_afs Lifecycle Management

### 6.1 When is all_afs Cleared?

**Location**: `aalta_formula.cpp:77`

```cpp
void aalta_formula::clear()
{
  for (auto f : all_afs)
    delete f;
  all_afs.clear();
}
```

**Call sites**:
- Per top-level formula processing
- Between DFA constructions
- Not during a single synthesis run

**Implication**: The global set can grow large during processing but is cleared between independent formulas.

### 6.2 Maximum Size Estimate

For a typical LTLf synthesis problem:
- Input formula: ~10-50 operators
- After NNF: ~2-5x expansion
- After XNF: ~2-3x expansion
- **Estimated unique formulas**: 100-1000

This is well within hash table capacity.

---

## 6.5 Tag Analysis: Decision to Remove

### 6.5.1 What is Tag?

```cpp
typedef std::list<aalta_formula*> tag_t;  // Tag is a list of formula pointers
tag_t *_tag;  // "Label, position information relative to Until"
```

**Purpose**: Track the Until nesting context of a formula.

```cpp
// In classify() function:
if (ret._op == Until) {
  until = true;
  tag->push_back(this->unique());  // Add this Until to tag
}
// ... process children with tag ...
if (until) tag->pop_back();  // Remove when exiting Until
```

### 6.5.2 Why Tag Can Be Removed

**Evidence**: The `classify()` function is **commented out** in current code:

```cpp
// aalta_formula.cpp:869
// clc_hash();
// *this = *simplify ();  // Old version
//*this = *classify ();    // New version commented out!
```

**Reasoning**:
1. Tag was used for Until-specific optimizations in an older version
2. Current implementation does not call `classify()`, so tag is always `NULL`
3. Including tag in hash computation is unnecessary overhead
4. New design focuses on NNF → XNF → simplify pipeline, not Until-specific handling

### 6.5.3 Decision

**Remove tag from new design**:
- No `_tag` member in `Formula` class
- Hash computation does not include tag
- Simpler, more maintainable design

**Impact**: None - tag is not used in current code path.

---

## 7. Comparison with Proposed New Design

### 7.1 Similarities

| Aspect | Original | New Design | Status |
|--------|----------|------------|--------|
| **Hash consing** | Yes | Yes | ✅ Same approach |
| **Hash table** | hash_set | std::unordered_set | ✅ Same underlying structure |
| **Cached hash** | Yes | Yes | ✅ Keep |
| **Structural equality** | Yes (via pointers) | Yes (via pointers) | ✅ Keep |
| **Immutability** | After unique() | From creation | ✅ Stronger in new |

### 7.2 Differences

| Aspect | Original | New Design | Impact |
|--------|----------|------------|--------|
| **Hash function** | Custom XOR mixing | std::hash (with backup) | Simpler, standard |
| **Pool scope** | Global (all_afs) | Per-DFA | Safer, clearer |
| **Memory ownership** | Manual delete | vector<unique_ptr> | RAII, no leaks |
| **Thread safety** | None | None (planned) | Not blocking |
| **Variable management** | String-based | ID-based with pre-declaration | BDD-ready |
| **Tag support** | Yes (unused) | **Removed** | Simpler design |

---

## 8. Recommendations

### 8.1 Keep Hash + Structural Comparison

**Decision**: Use both hash and structural equality check.

**Rationale**:
- Hash alone can have collisions
- Structural comparison is O(1) due to pointer equality
- Safety and correctness outweigh micro-optimization

### 8.2 Use Cached Hash

**Decision**: Store hash in Formula object, compute once.

**Implementation**:
```cpp
class Formula {
  size_t hash_;  // Cached hash value

public:
  size_t hash() const { return hash_; }
};
```

### 8.3 Hash Function Selection

**Primary Decision**: Use `std::hash` with `hash_combine` pattern.

**Backup Plan**: Original XOR mixing documented for rollback if issues found.

#### 8.3.1 Option A: std::hash (Primary)

```cpp
// Standard hash_combine pattern (Boost-style)
template<typename T>
inline void hash_combine(size_t& seed, const T& value) {
  seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Formula hash computation
size_t compute_hash(OpType op, Formula* left, Formula* right, int var_id) {
  size_t h = 0;
  hash_combine(h, static_cast<size_t>(op));
  // Use pointer values (not hash values) - canonicalized formulas have unique pointers
  hash_combine(h, reinterpret_cast<size_t>(left));
  hash_combine(h, reinterpret_cast<size_t>(right));
  hash_combine(h, static_cast<size_t>(var_id));
  return h;
}
```

**Advantages**:
- Standard library, well-tested
- Better avalanche effect with golden ratio constant (0x9e3779b9)
- Lower collision rate than simple XOR
- Industry-standard pattern

#### 8.3.2 Option B: XOR Mixing (Backup/Reference)

```cpp
// Original implementation's XOR mixing (documented for rollback)
size_t compute_hash_xor_backup(OpType op, Formula* left, Formula* right, int var_id) {
  const size_t HASH_INIT = 1315423911;
  size_t h = HASH_INIT;
  h = (h << 5) ^ (h >> 27) ^ static_cast<size_t>(op);
  if (left != nullptr)
    h = (h << 5) ^ (h >> 27) ^ reinterpret_cast<size_t>(left);
  if (right != nullptr)
    h = (h << 5) ^ (h >> 27) ^ reinterpret_cast<size_t>(right);
  h = (h << 5) ^ (h >> 27) ^ static_cast<size_t>(var_id);
  return h;
}
```

**Advantages**:
- Proven in production (original implementation)
- Simpler operation (just shifts and XORs)
- Fast execution

**Disadvantages**:
- Weaker avalanche effect
- Higher collision probability
- Custom code (maintenance burden)

#### 8.3.3 Comparison

| Metric | std::hash + hash_combine | XOR Mixing |
|--------|---------------------------|------------|
| **Collision rate** | Lower | Moderate |
| **Avalanche effect** | Strong | Weaker |
| **Performance** | Similar | Similar |
| **Maintainability** | Standard | Custom |
| **Production tested** | Industry | This codebase only |

**Key Insight**: Original implementation used `left->_hash` and `right->_hash` (child hash values), but using **pointer values** is sufficient because canonicalization ensures equal subformulas have identical pointers.

### 8.4 Use std::unordered_set

**Decision**: Use C++11 `std::unordered_set` instead of custom `hash_set`.

**Rationale**:
- Standard library, well-tested
- Compatible with C++20 target
- Same performance characteristics
- No external dependencies

### 8.5 Equality Function

**Decision**: Use structural comparison with pointer equality for children.

**Implementation**:
```cpp
struct FormulaEqual {
  bool operator()(const Formula* f1, const Formula* f2) const {
    return f1->op() == f2->op() &&
           f1->left() == f2->left() &&   // Pointer compare
           f1->right() == f2->right() && // Pointer compare
           f1->var_id() == f2->var_id();  // Only for literals
  }
};
```

---

## 9. Performance Expectations

### 9.1 Hash Consing Overhead

| Metric | Original | New Design | Expected |
|--------|----------|------------|----------|
| **Hash computation** | Custom XOR | std::hash | Similar or slightly better |
| **Lookup** | O(1) | O(1) | Same |
| **Equality check** | O(1) pointers | O(1) pointers | Same |
| **Memory overhead** | 16 bytes/formula | 8 bytes/hash | **Better** |

### 9.2 Expected Deduplication Ratio

Based on typical LTLf formulas:
- **Conservative estimate**: 30-50% deduplication
- **Optimistic estimate**: 50-80% deduplication
- **Net memory savings**: 20-60% despite hash table overhead

### 9.3 Hash Collision Probability

With 64-bit hash values:
- **Birthday paradox**: ~10% collision at 10^9 elements
- **Typical use**: < 10^5 elements
- **Conclusion**: Negligible collision rate

---

## 10. Open Questions Resolved

### Q1: Is hash comparison sufficient?
**Answer**: No. Use hash + structural comparison for safety.

**Reason**: While collisions are rare, they can occur. Structural comparison is cheap (O(1)) due to pointer equality.

### Q2: What hash function should we use?
**Answer**: Use `std::hash` with custom specialization or simple combine.

**Reason**:
- Standard library well-optimized
- Custom hash function maintenance burden
- Original XOR mixing has no clear advantage

### Q3: How large is unique_table_ in practice?
**Answer**: 100-1000 unique formulas per synthesis problem.

**Reason**:
- Input formulas typically 10-50 operators
- NNF/XNF expansion 2-5x
- Deduplication reduces growth

### Q4: Should we use hash-only or hash+structural?
**Answer**: Hash + structural comparison.

**Reason**:
- Structural comparison is O(1) with pointer equality
- Safety guarantee
- Minimal overhead

---

## 11. Implementation Notes for New Design

### 11.1 Hash Computation (Primary: std::hash)

```cpp
// hash_combine utility (Boost-style)
template<typename T>
inline void hash_combine(size_t& seed, const T& value) {
  seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// FormulaPool::create() - internal hash computation
size_t FormulaPool::compute_hash(OpType op, Formula* left, Formula* right, int var_id)
{
  size_t h = 0;
  hash_combine(h, static_cast<size_t>(op));
  // Use pointer values: canonicalization ensures equal formulas have identical pointers
  hash_combine(h, reinterpret_cast<size_t>(left));
  hash_combine(h, reinterpret_cast<size_t>(right));
  hash_combine(h, static_cast<size_t>(var_id));
  return h;
}
```

### 11.1.1 Hash Computation (Backup: XOR Mixing)

```cpp
// Original XOR mixing (for rollback if needed)
size_t FormulaPool::compute_hash_xor_backup(OpType op, Formula* left, Formula* right, int var_id)
{
  const size_t HASH_INIT = 1315423911;
  size_t h = HASH_INIT;
  h = (h << 5) ^ (h >> 27) ^ static_cast<size_t>(op);
  if (left != nullptr)
    h = (h << 5) ^ (h >> 27) ^ reinterpret_cast<size_t>(left);
  if (right != nullptr)
    h = (h << 5) ^ (h >> 27) ^ reinterpret_cast<size_t>(right);
  h = (h << 5) ^ (h >> 27) ^ static_cast<size_t>(var_id);
  return h;
}
```

### 11.2 Hash Set Definition

```cpp
// In FormulaPool class
struct FormulaHash {
  size_t operator()(const Formula* f) const {
    return f->hash();
  }
};

// FormulaEqual 用于 hash consing 的相等判断
// 必须用结构比较，因为 hash 可能碰撞
struct FormulaEqual {
  bool operator()(const Formula* f1, const Formula* f2) const {
    // 快速路径：指针相同（对已存储对象比较有效）
    if (f1 == f2) return true;

    // 核心逻辑：结构比较
    return f1->op() == f2->op() &&
           f1->left() == f2->left() &&       // 指针比较 (hash consing 保证)
           f1->right() == f2->right() &&     // 指针比较 (hash consing 保证)
           f1->var_id() == f2->var_id();     // 仅当 op==Literal 时有效
  }
};

using UniqueTable = std::unordered_set<Formula*, FormulaHash, FormulaEqual>;
```

### 11.2.1 FormulaEqual 的两种使用场景

**场景 1：查找（最常见）**
```cpp
Formula key(op, left, right, var_id, hash, 0);  // 临时对象，pool_index=0
auto it = unique_table_.find(&key);              // 调用 FormulaEqual
```
- `key` 是临时对象，指针与已存储对象不同
- `key.pool_index() = 0`，已存储对象的 `> 0`
- **必须用结构比较判断**

**场景 2：已存储对象比较（罕见）**
```cpp
// 某种原因需要比较两个已存储的公式
if (FormulaEqual{}(stored_f1, stored_f2)) { ... }
```
- 指针可能相同（同一对象）
- pool_index 可能相同

| 检查 | 场景1（查找） | 场景2（已存储） | 必要性 |
|------|---------------|-----------------|--------|
| `f1 == f2` | 永远 false | 可能 true | 优化（快速路径） |
| `pool_index 相同` | 永远 false | 可能 true | **不需要**（收益太小） |
| **结构比较** | **必需** | 可能被短路 | **必需**（核心逻辑） |

### 11.2.2 为什么不检查 pool_index？

```cpp
// 不推荐的做法
if (f1->pool_index() == f2->pool_index()) return true;
```

**原因**：
1. 查找场景（99%）：临时对象 `pool_index=0`，已存储对象 `> 0`，永远不匹配
2. 收益太小：只有比较已存储对象时才有用，这种情况很少
3. 增加复杂度：额外的检查分支，但几乎没有性能提升

**结论**：只加指针检查（`f1 == f2`），不加 pool_index 检查。

### 11.2.3 Formula 成员变量说明

```cpp
class Formula {
  OpType op_;          // 操作符类型 (And, Or, Not, Next, Until, Release, Literal等)
  Formula* left_;     // 左子公式指针
  Formula* right_;    // 右子公式指针
  int var_id_;        // 原子命题的ID (仅当 op_==Literal 时有效)
  size_t hash_;       // 缓存的哈希值
  size_t pool_index_; // 在 FormulaPool 中的下标 (用于排序，见 11.2.5)
};
```

**`var_id_` 说明**：
- 原始实现中，原子命题用 `_op` 字段存储 ID，`_left=_right=NULL`
- 新设计分离了操作符和变量ID：`op_=Literal, var_id_=ID`
- 判断是否为原子命题：`op_ == Literal`（比原实现的 `left==NULL && right==NULL` 更清晰）

### 11.2.4 Formula vs 原始 aalta_formula 对比

| 方面 | 原始 aalta_formula | 新 Formula |
|------|-------------------|------------|
| **原子命题存储** | `_op=ID, _left=NULL, _right=NULL` | `op_=Literal, var_id_=ID` |
| **判断是否原子** | `if (_left==NULL && _right==NULL)` | `if (op_==Literal)` |
| **获取原子ID** | 直接用 `_op` | 用 `var_id_` |
| **唯一标识** | `_id` (但 `<` 没用) | `pool_index_` (用于排序) |

### 11.2.5 排序用比较运算符 (pool_index 方案)

**问题**：原始实现的 `<` 运算符存在严格弱序问题

```cpp
// 原始实现 - 问题版本
bool aalta_formula::operator< (const aalta_formula& af) const {
  if (_left != af._left)  return _left < af._left;   // 指针比较
  if (_right != af._right) return _right < af._right;
  if (_op != af._op)      return _op < af._op;
  if (_tag != af._tag)    return _tag < af._tag;
  return false;
}
```

**问题分析**：
- 字典序比较：left → right → op → tag
- 指针地址在多次运行中不固定
- 原实现有 `_id` 字段但 `<` 没有使用

**解决方案：用 pool_index_ 排序**

```cpp
// Formula 类中的排序运算符
bool operator<(const Formula& other) const {
  return pool_index_ < other.pool_index_;
}
```

**优势**：
| 优势 | 说明 |
|------|------|
| **严格弱序** | 整数比较，完全满足 C++ 排序要求 |
| **唯一性** | 每个 index 唯一，不可能出现 a<b 和 b<a 同时成立 |
| **高效** | 单次整数比较 |
| **稳定** | index 一旦分配不会改变 |

### 11.3 Deduplication Flow

```cpp
Formula* FormulaPool::create(OpType op, Formula* left, Formula* right, int var_id)
{
  // 1. Compute hash for lookup key
  size_t h = compute_hash(op, left, right, var_id);

  // 2. Create temporary formula for lookup (pool_index=0, 无效值)
  Formula key(op, left, right, var_id, h, 0);

  // 3. Check if already exists
  auto it = unique_table_.find(&key);
  if (it != unique_table_.end()) {
    return *it;  // Return existing formula
  }

  // 4. Create new formula: 分配唯一的 pool_index
  size_t index = formulas_.size();  // 下一个可用的 index
  Formula* new_formula = new Formula(op, left, right, var_id, h, index);

  // 5. 添加到存储
  formulas_.emplace_back(new_formula);
  unique_table_.insert(new_formula);

  return new_formula;
}
```

**关键点**：
- 查找用的 `key` 使用 `pool_index=0`（不影响 hash 相等判断）
- 新公式分配唯一的 `pool_index = formulas_.size()`
- `pool_index` 一旦分配永不改变

### 11.4 两种比较方式的用途对比

| 比较方式 | 用途 | 比较内容 | 使用的场合 |
|----------|------|----------|-----------|
| **FormulaEqual** | hash consing 相等判断 | op + left/right 指针 + var_id | `unordered_set` 查找 |
| **operator<** | std::sort 排序 | pool_index_ | simplify 排序去重 |

---

## 12. Conclusions

### Summary

1. **Original implementation is sound**: Uses hash consing with hash+structural comparison
2. **Performance is good**: O(1) lookup, O(1) equality, cached hash
3. **Design transfers well**: New design can use same approach with improvements
4. **Key improvements in new design**:
   - Per-DFA scoping (not global)
   - RAII memory management
   - BDD-ready variable system
   - **Tag removed** (unused feature)
   - **std::hash** for hash computation (with XOR backup documented)

### Design Decisions Recorded

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Tag support** | Removed | Unused in current code, `classify()` commented out |
| **Hash function** | std::hash + hash_combine | Standard library, lower collision rate |
| **Hash backup** | XOR mixing documented | For rollback if issues found |
| **Hash input** | Pointer values | Canonicalization ensures unique pointers |
| **Equality check** | Hash + structural | Safety > micro-optimization |
| **Structural compare** | Pointer equality | O(1) due to canonicalization |
| **Sorting order** | pool_index_ | Strict weak ordering, fixes original `<` bug |
| **var_id_ field** | Separate from op_ | Type-safe, clearer than original `_op` reuse |
| **Modern C++ idioms** | Yes | RAII, standard library |

### TODO 2 Status: ✅ RESOLVED

**Action Items**:
- [x] Analyze `unique()` method
- [x] Analyze hash computation
- [x] Analyze `all_afs` set structure
- [x] Document findings
- [x] Provide recommendations

**Next Steps**:
- Implement hash consing in new FormulaPool
- Use std::unordered_set with cached hash
- Verify O(1) lookup performance in benchmarks

---

## References

- **Original code**: `lib/deps/formula/aalta_formula.cpp:1518-1545` (unique())
- **Original code**: `lib/deps/formula/aalta_formula.cpp:726-740` (clc_hash())
- **Original code**: `lib/deps/formula/aalta_formula.h:72-88` (hash/equality functors)
- **Original code**: `lib/deps/formula/aalta_formula.cpp:1005-1009` (operator==)
- **Hash set wrapper**: `lib/deps/util/hash_set.h`

---

**Document prepared by**: Claude Code analysis
**Date**: 2025-01-01
**Version**: 1.0
