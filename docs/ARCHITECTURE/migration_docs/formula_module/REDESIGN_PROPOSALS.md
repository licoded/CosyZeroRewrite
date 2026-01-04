# Formula Module Redesign Proposals

## Overview

This document presents **concrete redesign options** for the formula module (aalta_formula + af_utils), with trade-off analysis and implementation strategies.

**Goal**: Preserve all essential functionality (see CORE_FUNCTIONALITY.md) while fixing:
1. Memory management issues (raw pointers, leaks)
2. Thread safety (global mutable state)
3. Code complexity (duplication, O(n²) algorithms)
4. Performance bottlenecks

---

## Decision Matrix: High-Level Choices

### Choice 1: Memory Management Strategy

| Option | Pros | Cons | Effort | Thread-Safe | Memory |
|--------|------|------|--------|-------------|--------|
| **A. Shared Ptr** | Automatic GC, standard C++ | Ref count overhead | Low | Yes (atomic) | +16 bytes |
| **B. Unique Ptr** | Zero overhead, clear ownership | Manual cloning | Medium | Yes | 0 bytes |
| **C. Arena/Pool** | Fast alloc, cache-friendly | Complex lifecycle | High | Per-arena | -20 bytes |
| **D. Smart Pointer + Interning** | Fast equality, dedup | Complex, leak risk | High | Needs work | Variable |

**Recommended**: **Option B (Unique Ptr)** for simplicity and zero overhead
**Alternative**: **Option C (Arena)** if memory profiling shows allocation bottleneck

---

### Choice 2: Caching Strategy

| Option | Pros | Cons | Effort | Cache-Invalidation |
|--------|------|------|--------|-------------------|
| **A. Per-Context Cache** | Thread-safe, clearable | Pass context everywhere | Medium | Manual `clear()` |
| **B. Thread-Local Cache** | Fast, transparent | Memory N× threads | Low | Auto (thread end) |
| **C. No Caching (Recompute)** | Simplest, no cache issues | Slow for repeated work | Very Low | N/A |
| **D. LRU with Size Limit** | Bounded memory | Complex, evictions | High | Auto (LRU) |

**Recommended**: **Option A (Per-Context Cache)** for explicit control
**Alternative**: **Option B (Thread-Local)** if most work is single-threaded

---

### Choice 3: Formula Representation

| Option | Pros | Cons | Effort | Size | Equality Check |
|--------|------|------|--------|------|----------------|
| **A. Tree Nodes** | Simple, intuitive | Pointer chasing | Low | 32 bytes | O(n) traversal |
| **B. Flat Array** | Cache-friendly, fast | Complex indexing | Medium | 16 bytes | O(1) compare |
| **C. DAG with Hash Consing** | Auto dedup, O(1) eq | Global state issues | High | 40 bytes | O(1) pointer |
| **D. Immutable Trees** | Safe, shareable | Copy overhead | Medium | 24 bytes | O(1) if cached |

**Recommended**: **Option D (Immutable Trees)** with external hash-consing cache
**Alternative**: **Option C (DAG)** if profiling shows high duplication

---

## Detailed Proposals

### Proposal A: Minimal Cleanup (Conservative)

**Goal**: Fix critical bugs, keep architecture mostly unchanged

#### Changes

1. **Memory Management**: Replace raw `new/delete` with `std::unique_ptr`
   ```cpp
   // Before
   aalta_formula *f = new aalta_formula(...);
   delete f;

   // After
   auto f = std::make_unique<Formula>(...);
   ```

2. **Remove Global State**: Pass cache as parameter
   ```cpp
   // Before
   static hash_map<...> f_to_xnf;

   // After
   class FormulaCache {
       hash_map<...> xnf_cache_;
       Formula* get_xnf(Formula* f);
   };
   ```

3. **Fix Memory Leaks**: Add cache clearing
   ```cpp
   void FormulaCache::clear() {
       xnf_cache_.clear();
       unique_cache_.clear();
   }
   ```

4. **Remove Unused Code**: Delete functions not used in target config
   - Tag system
   - Sat/LtlF-specific code
   - Duplicate sorting functions

#### Pros
- **Effort**: 2-3 weeks
- **Risk**: Low (minimal architecture change)
- **Compatibility**: Easy to migrate existing code

#### Cons
- **Still has**: O(n²) algorithms in simplify
- **Still has**: Code duplication (simplify_and_weak vs merge_and)
- **Performance**: Limited improvement

#### Code Size
- Before: ~5,200 lines
- After: ~3,500 lines (33% reduction)

---

### Proposal B: Modern C++ Redesign (Balanced) ⭐ RECOMMENDED

**Goal**: Modernize architecture while preserving algorithms

#### Architecture

```cpp
namespace formula {

// Immutable formula (no mutations after construction)
class Formula {
public:
    // Factory methods (handle canonicalization)
    static Formula* create(int op, Formula* left = nullptr, Formula* right = nullptr);
    static Formula* from_string(const std::string& ltlf);

    // Accessors (pure, no side effects)
    int oper() const { return op_; }
    Formula* left() const { return left_; }
    Formula* right() const { return right_; }

    // Transformations (return new formulas, don't modify this)
    Formula* nnf() const;
    Formula* simplify() const;
    std::string to_string() const;

    // Comparison (structural)
    bool equals(const Formula* other) const;
    size_t hash() const;

private:
    Formula(int op, Formula* left, Formula* right);  // Private ctor

    int op_;
    Formula* left_;
    Formula* right_;
    size_t hash_;  // Computed once at construction

    // No _unique, _simp, _tag fields (externalized)
};

// Separate cache manager (not embedded in Formula)
class FormulaCache {
public:
    // Get canonical version or create new
    Formula* canonicalize(Formula* f);

    // Get cached transformation result
    Formula* get_xnf(Formula* f);
    Formula* get_nnf(Formula* f);
    Formula* get_simplified(Formula* f);

    // Cache management
    void clear();
    size_t size() const;

private:
    std::unordered_map<Formula*, Formula*> unique_cache_;
    std::unordered_map<Formula*, Formula*> xnf_cache_;
    std::unordered_map<Formula*, Formula*> nnf_cache_;
    std::unordered_map<Formula*, Formula*> simplify_cache_;
};

// Core operations (free functions, not methods)
Formula* xnf_with_tail(Formula* f, FormulaCache& cache);
Formula* rmnext(
    Formula* predecessor,
    Formula* edge,
    const std::unordered_set<int>& all_vars,
    FormulaCache& cache
);
std::vector<Formula*> get_and_sub_formulas(Formula* f);

} // namespace formula
```

#### Key Design Decisions

1. **Immutability**: Formulas never change after construction
   - Safe to share between threads
   - Easy to reason about
   - Enables structural sharing

2. **External Cache**: Cache managed separately, not in formulas
   - Thread-safe (each thread has its own cache)
   - Explicit lifetime (can clear when needed)
   - No per-formula overhead

3. **Factory Pattern**: `Formula::create()` handles canonicalization
   - Ensures structural uniqueness
   - Fast pointer equality
   - Hidden from users

4. **Free Functions for Operations**: `xnf_with_tail()`, `rmnext()` not methods
   - Clear separation of concerns
   - Easy to test
   - Cache passed explicitly

#### Algorithm Improvements

1. **Simplify with HashSet** (O(n²) → O(n)):
   ```cpp
   // Before: O(n²) pairwise comparison
   for (i = 0; i < n; ++i)
       for (j = i+1; j < n; ++j)
           if (is_conflict(afp[i], afp[j]))
               return FALSE();

   // After: O(n) hash set lookup
   std::unordered_set<int> seen_literals;
   for (auto* lit : literals) {
       if (seen_literals.contains(-lit))
           return FALSE();
       seen_literals.insert(lit);
   }
   ```

2. **Merge Duplicate Functions** (simplify_and_weak ≈ merge_and):
   ```cpp
   // Single function with strategy parameter
   enum class MergeStrategy { Strict, Weak };
   Formula* merge_and_formulas(
       const std::vector<Formula*>& formulas,
       MergeStrategy strategy
   );
   ```

3. **Use std::vector Instead of Raw Arrays**:
   ```cpp
   // Before
   aalta_formula **afp = new aalta_formula*[af_list.size () + 1];
   // ... use afp ...
   delete[] afp;

   // After
   std::vector<Formula*> afp;
   afp.reserve(af_list.size());
   // ... use afp ...
   // Automatic cleanup
   ```

#### Pros
- **Effort**: 4-6 weeks
- **Risk**: Medium (architecture change, but well-tested algorithms)
- **Performance**: 2-3× faster in hot paths
- **Thread-safe**: Yes (explicit cache per thread)
- **Memory**: 40% less per formula (no _unique, _simp, _tag)

#### Cons
- **Requires testing**: Need comprehensive test suite
- **Learning curve**: New API for existing code

#### Code Size
- Before: ~5,200 lines
- After: ~2,800 lines (46% reduction)

---

### Proposal C: High-Performance Redesign (Aggressive)

**Goal**: Maximize performance, optimize hot paths

#### Architecture

```cpp
namespace formula {

// Packed formula representation (24 bytes)
struct Formula {
    uint32_t hash;      // Hash for fast comparison
    uint32_t op_data;   // Operator + metadata (flags, etc.)
    uint64_t left_ptr;  // Packed pointer (or offset in arena)
    uint64_t right_ptr; // Packed pointer

    // Inline operations for speed
    int oper() const { return op_data & 0xFF; }
    Formula* left() const { return unpack(left_ptr); }
    Formula* right() const { return unpack(right_ptr); }
};

// Arena allocator for cache-friendly allocation
class FormulaArena {
public:
    Formula* allocate(int op, Formula* left, Formula* right);
    void reset();  // Clear all allocations at once

private:
    std::vector<std::unique_ptr<uint8_t[]>> chunks_;
    size_t offset_in_chunk_;
};

// Global interning table (read-only after construction)
class FormulaInternTable {
public:
    Formula* intern(Formula* f);  // Return canonical version
    Formula* find_equal(Formula* f);  // O(1) lookup

private:
    std::unordered_map<size_t, std::vector<Formula*>> hash_table_;
};

} // namespace formula
```

#### Advanced Optimizations

1. **Pointer Packing** (store small integers as tagged pointers):
   ```cpp
   // Encode True/False/TAIL/NOT_TAIL as small integers
   constexpr uintptr_t TAG_MASK = 0x3;
   constexpr uintptr_t PTR_MASK = ~TAG_MASK;

   Formula* unpack(uint64_t ptr) {
       if (ptr & TAG_MASK) {
           return reinterpret_cast<Formula*>(ptr << 48);  // Embedded value
       }
       return reinterpret_cast<Formula*>(ptr);
   }
   ```

2. **SIMD for Formula Comparison** (compare multiple formulas at once):
   ```cpp
   // Use AVX2 to compare 4 formulas in parallel
   __m256i hash_vec = _mm256_loadu_si256((__m256i*)hashes);
   __m256i target_vec = _mm256_set1_epi32(target_hash);
   __m256i cmp = _mm256_cmpeq_epi32(hash_vec, target_vec);
   ```

3. **Memoization with Perfect Hashing**:
   ```cpp
   // Use perfect hash for common operator combinations
   constexpr size_t PERFECT_HASH_SIZE = 1024;
   Formula* perfect_hash_table[PERFECT_HASH_SIZE];

   size_t perfect_hash(int op, Formula* left, Formula* right) {
       return ((op * 31) + (size_t)left * 17 + (size_t)right) % PERFECT_HASH_SIZE;
   }
   ```

4. **Custom Allocator for Small Objects**:
   ```cpp
   // Specialized allocator for Formula nodes
   template<typename T>
   class FormulaAllocator {
       // Use memory pool for fast allocation
       // Free list for recycled nodes
       // Thread-local storage for parallel allocation
   };
   ```

#### Pros
- **Effort**: 8-12 weeks
- **Performance**: 5-10× faster in hot paths
- **Memory**: 60% less per formula
- **Cache-friendly**: Better locality

#### Cons
- **Complexity**: High (custom allocators, pointer packing)
- **Debugging**: Harder (memory tricks)
- **Portability**: May have platform-specific code

#### Code Size
- Before: ~5,200 lines
- After: ~3,200 lines (38% reduction, but more complex)

---

## Comparison Table

| Aspect | Proposal A (Minimal) | Proposal B (Balanced) | Proposal C (Aggressive) |
|--------|---------------------|----------------------|------------------------|
| **Effort** | 2-3 weeks | 4-6 weeks | 8-12 weeks |
| **Risk** | Low | Medium | High |
| **Performance gain** | 1.2× | 2-3× | 5-10× |
| **Memory reduction** | 20% | 40% | 60% |
| **Thread-safe** | Yes | Yes | Yes |
| **Code size** | 3.5k lines | 2.8k lines | 3.2k lines |
| **Complexity** | Low | Medium | High |
| **Maintainability** | Good | Very Good | Fair |
| **Testability** | Good | Very Good | Fair |

---

## Recommendation and Discussion Points

### My Recommendation: Proposal B (Balanced) ⭐

**Reasons**:
1. **Best risk/reward ratio**: Significant improvements without excessive complexity
2. **Modern C++ practices**: Immutable data, external cache, factory pattern
3. **Maintainable**: Clear architecture, easy to understand and modify
4. **Testable**: Pure functions, explicit dependencies
5. **Scalable**: Can add optimizations from Proposal C later if needed

**Timeline**:
- Week 1-2: Implement Formula class, FormulaCache
- Week 3-4: Port transformations (nnf, xnf_with_tail, simplify)
- Week 5: Port rmnext, utilities
- Week 6: Testing, integration, performance tuning

---

### User Constraints and Requirements

**Important**: The following constraints were provided during discussion and must be considered:

1. **Multi-threading**: Not a concern - single-threaded optimization only
   - Remove thread-safety considerations from proposals
   - Simplify cache design (no atomic operations needed)

2. **Future BDD Replacement**: Current deduplication approach may be replaced with BDD
   - Keep good abstraction to facilitate future BDD replacement
   - But don't over-abstract - BDD optimization is a major change
   - API incompatibility is acceptable when migrating to BDD

3. **Design Priority**: Clean architecture over micro-optimizations
   - Prefer simple, maintainable code
   - Only use advanced techniques (pointer packing, etc.) if clearly beneficial

4. **Excluded Advanced Techniques** (User Decision - 2025-01-01):
   - **Arena Allocators**: NOT using in current implementation
     - Reason: Avoid complexity of custom memory management
     - Alternative: Standard smart pointers (unique_ptr/shared_ptr)
   - **Pointer Packing**: NOT using in current implementation
     - Reason: Complexity not worth minimal memory savings
     - Alternative: Allocate constants as normal objects
   - **Documentation**: These techniques remain in `ADVANCED_OPTIMIZATION_TECHNIQUES.md` for educational reference only

---

### Discussion Points for You

The following points were raised during initial discussion and are documented for reference:

#### 1. Memory Management Preference

**Question**: Do you prefer shared_ptr (automatic GC) or unique_ptr (manual cloning)?

**Trade-off**:
```cpp
// Shared ptr: Easy, but slower
using FormulaPtr = std::shared_ptr<Formula>;
FormulaPtr f = Formula::create(And, left, right);
// Ref count automatically managed

// Unique ptr: Fast, but need clone() when sharing
using FormulaPtr = std::unique_ptr<Formula>;
FormulaPtr f = Formula::create(And, left, right);
FormulaPtr f2 = f->clone();  // Explicit copy when needed
```

**My suggestion**: Start with `unique_ptr` for performance, add `clone()` method

---

#### 2. Cache Invalidation Strategy

**Question**: When should caches be cleared?

**Options**:
- **A. Per-formula**: Clear cache after each formula processed
  - Pros: Memory bounded
  - Cons: Loses cross-formula sharing

- **B. Per-DFA**: Clear cache after each DFA built
  - Pros: Shares within one synthesis problem
  - Cons: Memory grows with DFA size

- **C. Manual**: User calls `cache.clear()`
  - Pros: Full control
  - Cons: Easy to forget

**My suggestion**: **Option B (Per-DFA)** - clear cache in `compositional_synthesis1()` after each `getAndSubAfs` iteration

---

#### 3. Error Handling

**Question**: How should errors be handled?

**Options**:
- **A. Exceptions**: `throw std::runtime_error("rmnext: not in XNF")`
  - Pros: Standard C++, explicit error handling
  - Cons: Can be slow, exception safety required

- **B. Error codes**: `return Result<Formula*, Error>`
  - Pros: Explicit, no exception overhead
  - Cons: Verbose, easy to ignore

- **C. Assertions**: `assert(formula->in_xnf() && "must be in XNF")`
  - Pros: Fast in release, clear in debug
  - Cons: Crashes in release if assertion fails

**My suggestion**: **Assertions in debug**, **Error codes in public API**

---

#### 4. Testing Strategy

**Question**: How will we test the redesigned code?

**Critical areas**:
1. **Correctness**: Ensure transformations produce same results
   - Property-based testing (generate random formulas)
   - Round-trip tests (parse → transform → print → parse)

2. **Performance**: Benchmark hot paths
   - rmnext on various formula sizes
   - xnf_with_tail on nested Until/Release
   - simplify on large AND/OR chains

3. **Memory**: Check for leaks
   - Valgrind / AddressSanitizer
   - Cache growth over time

**My suggestion**: Create `tests/formula_test.cpp` with:
- 100+ test cases covering all operators
- Performance benchmarks (baseline current code)
- Memory leak detection

---

#### 5. Migration Strategy

**Question**: Should we migrate incrementally or all-at-once?

**Options**:
- **A. Incremental**: Replace one module at a time
  - Keep old code alongside new
  - Migrate: parsing → nnf → xnf → rmnext
  - Pros: Low risk, easy to test each step
  - Cons: Duplicate code during transition

- **B. All-at-once**: Rewrite everything in one branch
  - Pros: Clean slate, no compromises
  - Cons: High risk, big integration step

**My suggestion**: **Incremental migration**:
1. Week 1-2: New Formula class (keep old aalta_formula)
2. Week 3-4: New transformations (test against old)
3. Week 5: Switch synthesis code to use new API
4. Week 6: Remove old code

---

## Next Steps

### If You Choose Proposal B (Recommended):

1. **Create new files** (alongside old code):
   ```
   lib/include/formula/formula.h          (new)
   lib/include/formula/formula_cache.h    (new)
   lib/include/formula/formula_ops.h      (new)
   lib/deps/formula/formula.cpp           (new)
   lib/deps/formula/formula_ops.cpp       (new)
   ```

2. **Write tests first** (TDD approach):
   ```cpp
   // tests/formula_test.cpp
   TEST(Formula, nnf_pushes_negation) {
       auto* f = Formula::from_string("!(p & q)");
       auto* nnf_f = f->nnf();
       EXPECT_EQ(nnf_f->to_string(), "!p | !q");
   }
   ```

3. **Implement in order**:
   - Formula class + FormulaCache
   - nnf, simplify, xnf_with_tail
   - rmnext, utilities
   - Integration with synthesis code

4. **Performance comparison**:
   ```bash
   ./benchmarks_old  # Baseline with old code
   ./benchmarks_new  # Compare with new code
   ```

### If You Want to Discuss Further:

I can provide:
1. **Detailed API design** for any proposal
2. **Implementation sketches** for specific functions
3. **Performance analysis** of specific hot paths
4. **Testing strategy** for correctness guarantees

**Which aspects would you like to discuss in more detail?**
