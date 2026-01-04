# Formula Module Rewrite - Design Documentation

**Status**: ✅ Design Complete, Ready for Implementation

**Date**: 2025-01-01

---

## Overview

This directory contains the complete design specification for rewriting the Formula module (`aalta_formula` and `af_utils`) in modern C++20.

---

## Document Navigation

### Start Here 📖

**[FORMULA_REWRITE_DESIGN.md](./FORMULA_REWRITE_DESIGN.md)** - Complete Design Specification
- **What**: Comprehensive design document with all details
- **Who**: For implementers and reviewers
- **Content**: Architecture, API specifications, class diagrams, testing strategy
- **Length**: ~1500 lines
- **Read time**: 30-45 minutes

### Visual Learners 🎨

**[FLOWCHARTS.md](./FLOWCHARTS.md)** - Flowcharts and Visual Diagrams
- **What**: ASCII art flowcharts for all key processes
- **Who**: For visual thinkers
- **Content**: Formula lifecycle, transformations, memory management
- **Length**: ~500 lines
- **Read time**: 20-30 minutes

### Implementers 🔨

**[IMPLEMENTATION_TASKS.md](./IMPLEMENTATION_TASKS.md)** - Task Breakdown
- **What**: Detailed task list with time estimates
- **Who**: For AI assistant or developers implementing
- **Content**: Phase-by-phase tasks, dependencies, acceptance criteria
- **Length**: ~400 lines
- **Read time**: 15-20 minutes

### Critical TODOs ⚠️ MUST READ FIRST!

**[TODOs.md](./TODOs.md)** - Open Questions and Blocking Issues
- **✅ TODO 1**: NNF and XNF understanding (**RESOLVED & DOCUMENTED**)
  - See "Strong Next Negation in Finite Traces" section for NNF solution
  - See "XNF Transformation Strategy" section for XNF solution
  - NNF rules: [NNF_TRANSFORMATION.md](./NNF_TRANSFORMATION.md) - Complete specification (~500 lines)
  - XNF rules: [XNF_TRANSFORMATION.md](./XNF_TRANSFORMATION.md) - Complete specification (~600 lines)
  - Summary: [FORMULA_REWRITE_DESIGN.md](./FORMULA_REWRITE_DESIGN.md) - NNF and XNF sections
- **✅ TODO 2**: Hash consing efficiency analysis (**RESOLVED & DOCUMENTED**)
  - See [HASH_CONSING_ANALYSIS.md](./HASH_CONSING_ANALYSIS.md) - Complete analysis (~500 lines)
  - Findings: O(1) lookup, cached hash, hash+structural comparison recommended
- **✅ TODO 3**: Simplification algorithm details verification (**RESOLVED & DOCUMENTED**)
  - See [SIMPLIFY_ANALYSIS.md](./SIMPLIFY_ANALYSIS.md) - Complete analysis (~820 lines)
  - Findings: Iterative flattening, O(n log n) actual (not O(n²) as thought), HashSet can improve to O(n)
  - Critical discovery: `is_conflict()` is disabled (always returns false)
- Secondary TODOs: Lifecycle, error handling, validation
- Resolution process and next steps
- **IMPORTANT**: All blocking TODOs resolved! Secondary TODOs are non-blocking.

### Background & Rationale 📚

These documents explain WHY we made certain design decisions:

**[MEMORY_MANAGEMENT_DECISION.md](../formula_module/MEMORY_MANAGEMENT_DECISION.md)**
- FormulaPool with raw pointers
- Per-DFA scoping
- Ownership model

**[ATOMIC_VARIABLE_HANDLING.md](../formula_module/ATOMIC_VARIABLE_HANDLING.md)**
- Pre-allocated variables
- Output-first ordering
- Auto-extraction for testing

**[PROPOSAL_B_ARCHITECTURE.md](../formula_module/PROPOSAL_B_ARCHITECTURE.md)**
- Immutable formulas
- External cache management
- O(n) simplification

**[SIMPLIFY_IMPLEMENTATION.md](../formula_module/SIMPLIFY_IMPLEMENTATION.md)**
- Current implementation analysis
- Optimization opportunities
- O(n²) → O(n) transformation

**[FORMULA_OPERATIONS.md](../formula_module/FORMULA_OPERATIONS.md)**
- LTLf theoretical foundations
- Transformation pipeline (NNF, XNF, simplify, rmnext)
- Formula progression semantics

---

## Quick Reference

### Key Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Language** | C++20 | Existing expertise, direct reuse |
| **Formula immutability** | Yes | Safe sharing, easier reasoning |
| **Memory management** | FormulaPool with raw pointers | Per-DFA scoping, high performance |
| **Variable allocation** | Pre-allocated from partition | BDD-ready, fixed ordering |
| **Variable ordering** | Outputs first, then inputs | System-centric perspective |
| **Canonicalization** | Hash consing | Automatic deduplication |
| **Simplify complexity** | O(n) with HashSet | 100-500× speedup |
| **Backward compatibility** | Yes (auto-extraction) | Support testing without partitions |

### Core Components

```
FormulaPool (Owner)
  ├── Variable Management
  │   ├── declare_variables()
  │   ├── get_variable_id()
  │   └── extract_variables_from_formula()
  │
  ├── Canonicalization Cache
  │   └── unique_table_ (unordered_set)
  │
  └── Formula Ownership
      └── formulas_ (vector<unique_ptr<Formula>>)

Formula (Immutable)
  ├── OpType op_
  ├── Formula* left_   (reference)
  ├── Formula* right_  (reference)
  ├── int var_id_      (only for Literal)
  └── size_t hash_     (cached)

Operations (return NEW Formula*)
  ├── nnf()
  ├── simplify()
  ├── xnf_with_tail()
  └── rmnext()
```

### Variable Declaration Flow

```
Option 1: With Partition (Normal)
  Read .part file → declare_variables() → parse formula

Option 2: Without Partition (Testing)
  Parse formula → extract_variables() → auto-declare as outputs
```

### Transformation Pipeline

```
Parse → NNF → Simplify → XNF → (rmnext during DFA construction)
```

---

## Implementation Checklist

### Phase 1: Foundation (Week 1)
- [ ] Create directory structure
- [ ] Implement Formula class (basic)
- [ ] Implement FormulaPool class (skeleton)
- [ ] Implement hash consing

### Phase 2: Variable Management (Week 1-2)
- [ ] Implement variable declaration
- [ ] Implement variable creation
- [ ] Implement partition file loading
- [ ] Implement auto-extraction

### Phase 3: Formula Operations (Week 2)
- [ ] Implement NNF transformation
- [ ] Implement simplification (O(n))
- [ ] Implement XNF transformation
- [ ] Implement rmnext (progression)

### Phase 4: Integration & Testing (Week 2)
- [ ] Write unit tests
- [ ] Write integration tests
- [ ] Run benchmarks

### Phase 5: Documentation & Cleanup (Week 2)
- [ ] Add documentation
- [ ] Code review & refactoring
- [ ] Final testing

**Estimated timeline**: 2 weeks (full-time)

---

## Success Criteria

The rewrite is successful when:

1. ✅ **Correctness**: Passes all test cases from original implementation
2. ✅ **Performance**: Within 1.5× of original (or faster)
3. ✅ **Memory**: No memory leaks (Valgrind clean)
4. ✅ **Code quality**: Well-documented, tested, maintainable
5. ✅ **Compatibility**: Produces same results as original

---

## Files to Replace

**Original** → **New**

```
lib/deps/formula/aalta_formula.h → include/formula/formula.hpp
lib/deps/formula/aalta_formula.cpp → src/formula/formula.cpp
lib/deps/formula/af_utils.h → include/formula/formula_ops.hpp
lib/deps/formula/af_utils.cpp → src/formula/nnf.cpp
                           → src/formula/xnf.cpp
                           → src/formula/simplify.cpp
                           → src/formula/rmnext.cpp
```

---

## Design Principles

1. **Simplicity over cleverness**: Prefer clear, understandable code
2. **Performance matters**: O(n) algorithms, hash tables, minimal allocations
3. **Safety first**: Immutable formulas, RAII, clear ownership
4. **Testability**: Unit tests for every component
5. **Document decisions**: Explain why, not just what

---

## Questions?

### Common Questions

**Q: Why raw pointers instead of smart pointers?**
A: Performance and user preference. FormulaPool owns all formulas, so no need for reference counting.

**Q: Why immutable formulas?**
A: Safe sharing, easier reasoning, thread-safe, no cache invalidation issues.

**Q: What if I don't have a partition file?**
A: Use `extract_variables_from_formula()` - automatically extracts all variables as outputs.

**Q: Can I change a formula after creation?**
A: No, formulas are immutable. Create a new formula with `pool->create()`.

**Q: How do I check if two formulas are equal?**
A: Use pointer equality (`f1 == f2`) - canonicalization guarantees structural uniqueness.

**Q: What's the performance improvement expected?**
A: Simplify: 100-500× faster (O(n²) → O(n)). Overall: 1.5-2× faster.

**Q: Will this work with BDD migration?**
A: Yes! Pre-allocated variables with fixed ordering are BDD-ready.

---

## Next Steps

1. **Review** [FORMULA_REWRITE_DESIGN.md](./FORMULA_REWRITE_DESIGN.md) for complete design
2. **Visualize** key processes with [FLOWCHARTS.md](./FLOWCHARTS.md)
3. **Plan** implementation using [IMPLEMENTATION_TASKS.md](./IMPLEMENTATION_TASKS.md)
4. **Start coding** following phase-by-phase tasks
5. **Test** thoroughly with provided test cases
6. **Benchmark** against original implementation

---

## Appendix: Related Documentation

### Formula Module Analysis (Background)

- [CORE_FUNCTIONALITY.md](../formula_module/CORE_FUNCTIONALITY.md) - What to keep from original
- [REDESIGN_PROPOSALS.md](../formula_module/REDESIGN_PROPOSALS.md) - 3 redesign proposals
- [ADVANCED_OPTIMIZATION_TECHNIQUES.md](../formula_module/ADVANCED_OPTIMIZATION_TECHNIQUES.md) - Pointer packing, arena allocators (not used)

### System Architecture (Context)

- [COMB_FULL_1_COMP_IDX_1_ANALYSIS.md](../system_architecture/COMB_FULL_1_COMP_IDX_1_ANALYSIS.md) - How formulas are used
- [EDGE_CONSTRAINT_SPEC.md](../system_architecture/EDGE_CONSTRAINT_SPEC.md) - BDD/ADD usage
- [FLOWCHART.md](../system_architecture/FLOWCHART.md) - Overall system flow

### Reimplementation Planning

- [NEW_REPOSITORY_SPEC.md](../reimplementation/NEW_REPOSITORY_SPEC.md) - Overall project plan

---

## Contact

**Design by**: Claude Code + User collaboration
**Date**: 2025-01-01
**Version**: 1.0

**Ready for implementation! 🚀**
