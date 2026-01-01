# Cosy Documentation Index

This document provides an organized index of all documentation in the `docs/` directory.

---

## Directory Structure

```
docs/
├── formula_module/           # Formula module (aalta_formula + af_utils) documentation
├── system_architecture/      # System architecture and flow documentation
├── formula_redesign/         # Formula module rewrite design (NEW!)
├── reimplementation/         # Reimplementation planning documents
└── README.md                 # This file
```

---

## Formula Module Documentation (`formula_module/`)

Documentation related to the formula module (aalta_formula and af_utils), including analysis, redesign proposals, and optimization techniques.

### Core Analysis and Extraction

**[CORE_FUNCTIONALITY.md](formula_module/CORE_FUNCTIONALITY.md)** - Essential Functionality to Keep
- Analyzes 5,232 lines across aalta_formula and af_utils
- Identifies core API: Formula AST, transformations (nnf, xnf_with_tail, simplify)
- Critical operations: rmnext (state progression), getAndSubAfs (decomposition)
- Lists what to remove: tag system, global caching, complex simplify rules
- Minimal API: 70% reduction in public surface
- Hot path analysis and performance requirements

**[FORMULA_OPERATIONS.md](formula_module/FORMULA_OPERATIONS.md)** - Formula Operations: rmnext and aalta_formula
- Theoretical foundations of LTLf semantics
- Transformation pipeline: parse → nnf → simplify → xnf_with_tail → rmnext
- Detailed explanation of rmnext (Formula Progression)
- Connection between theory and implementation
- Complexity analysis and usage examples

**[SIMPLIFY_IMPLEMENTATION.md](formula_module/SIMPLIFY_IMPLEMENTATION.md)** - Simplify Series Implementation Analysis
- Comprehensive analysis of 6 simplify functions
- Helper functions: split, contain, mutex
- Memory management issues identified
- Code duplication problems
- 10 optimization suggestions with code examples

**[LTLf_BASIC.md](formula_module/LTLf_BASIC.md)** - LTLf Theoretical Foundations (User-contributed)
- LTLf definition and syntax
- Strong Next vs Weak Next semantics
- Normal forms: NNF, TNF, XNF
- LTLf transition system construction
- Finite trace satisfiability

---

### Redesign and Optimization

**[REDESIGN_PROPOSALS.md](formula_module/REDESIGN_PROPOSALS.md)** - Formula Module Redesign Proposals
- Decision matrix for high-level choices
- 3 detailed proposals (Minimal/Balanced/Aggressive)
- Recommendation: Proposal B (Balanced) with immutable formulas
- Discussion points for user consideration
- Migration strategy and next steps

**[MEMORY_MANAGEMENT_DECISION.md](formula_module/MEMORY_MANAGEMENT_DECISION.md)** - Memory Management Design Decision ⭐ FINAL DECISION
- Analysis of current implementation (global all_afs, manual management)
- User's key insight: Single DFA context (simplifies design significantly)
- User's key insight: Sub-formula sharing is the priority
- Final design: FormulaPool with raw pointers (NOT smart pointers)
- Architecture: FormulaPool (owns) → Formula (tree) → DFAState (references)
- Benefits: Automatic deduplication, scoped lifecycle, clear ownership
- Migration guide from current implementation

**[ATOMIC_VARIABLE_HANDLING.md](formula_module/ATOMIC_VARIABLE_HANDLING.md)** - Atomic Variable Handling Design Decision ⭐ FINAL DECISION
- Analysis of current variable allocation (on-demand, global state)
- Comparison: treeFormula vs bddFormula variable strategies
- Final decision: Pre-allocate variables from partition file (Option B)
- Two-phase parsing: declare variables, then parse formula
- Fixed ordering: inputs first (ID 0..n-1), then outputs (ID n..n+m-1)
- Operators use enum type (separate from variable IDs)
- BDD-ready design with per-DFA variable scoping
- Implementation plan and code examples

**[PROPOSAL_B_ARCHITECTURE.md](formula_module/PROPOSAL_B_ARCHITECTURE.md)** - Proposal B: Detailed Architecture Design
- **Part 1: Immutable Formulas** - Thread safety, safe sharing, easier reasoning
- **Part 2: External Cache Management** - FormulaCache class, canonicalization, caching strategies
- **Part 3: Factory Pattern** - Formula::create, validation, optimization
- **Part 4: O(n²) → O(n) Simplify** - HashSet optimization, 100-500× speedup
- **Part 5: Merging Duplicate Functions** - Eliminate 80% code duplication
- **Part 6: Advanced Techniques** - Pointer packing and arena allocators explained
- Implementation timeline and expected improvements

**[ADVANCED_OPTIMIZATION_TECHNIQUES.md](formula_module/ADVANCED_OPTIMIZATION_TECHNIQUES.md)** - Advanced Optimization Techniques Explained
- **Educational reference only** - NOT used in current implementation
- **Pointer Packing** (指针压缩): Using unused bits in pointers for small values
- **Arena Allocators** (Arena分配器): Bulk allocation for speed and cache locality
- Visual representations, code examples, performance comparisons
- User decision: These techniques are excluded to keep design simple
- **Actual implementation**: Uses FormulaPool with raw pointers (see MEMORY_MANAGEMENT_DECISION.md)

**[BDD_CONSIDERATIONS.md](formula_module/BDD_CONSIDERATIONS.md)** - Future BDD Replacement Planning
- Current approach: Hash consing (structural sharing)
- Future approach: BDD (Binary Decision Diagrams)
- Design principle: Don't over-abstract - accept API incompatibility
- Migration strategy: Phase 1 (tree) → Phase 2 (evaluate BDD) → Phase 3 (migrate if beneficial)
- Variable ordering, TAIL marker, caching strategy considerations

---

## Formula Redesign Documentation (`formula_redesign/`)

**Complete design specification for rewriting the Formula module.**

**[README.md](formula_redesign/README.md)** - Formula Redesign Index
- Overview of all redesign documents
- Quick reference guide
- Implementation checklist
- Navigation guide

**[FORMULA_REWRITE_DESIGN.md](formula_redesign/FORMULA_REWRITE_DESIGN.md)** - Complete Design Specification ⭐ START HERE
- Comprehensive design document (~1500 lines)
- Architecture overview with ASCII diagrams
- Core components: Formula, FormulaPool
- API specifications with examples
- Implementation details
- Sequence diagrams, class diagrams
- Edge cases and error handling
- Testing strategy

**[FLOWCHARTS.md](formula_redesign/FLOWCHARTS.md)** - Flowcharts and Visual Diagrams
- Formula lifecycle (creation → transformation → destruction)
- Transformation pipeline (Parse → NNF → Simplify → XNF → rmnext)
- Variable declaration flows (with/without partition file)
- Canonicalization process (hash consing)
- Simplification algorithm (O(n) with HashSet)
- Memory management (RAII, ownership)

**[IMPLEMENTATION_TASKS.md](formula_redesign/IMPLEMENTATION_TASKS.md)** - Task Breakdown for Implementation
- 18 detailed tasks across 5 phases
- Time estimates: 53-71 hours total (~2 weeks)
- Dependencies between tasks
- Acceptance criteria for each task
- Testing requirements
- Notes for AI assistant

---

## System Architecture Documentation (`system_architecture/`)

Documentation describing the complete Cosy system architecture, algorithms, and data flow.

### Complete System Flow

**[COMB_FULL_1_COMP_IDX_1_ANALYSIS.md](system_architecture/COMB_FULL_1_COMP_IDX_1_ANALYSIS.md)** - Complete Code Flow Analysis
- Entry points and main flow
- Incremental composition loop
- Individual and combined DFA synthesis
- Tarjan SCC algorithm
- Edge constraint construction
- Backward search propagation
- Key data structures: Syn_Frame, EdgeCons, DfaComb_Frame

**[FLOWCHART.md](system_architecture/FLOWCHART.md)** - Visual Flowcharts
- High-level system flow
- Tarjan algorithm flow
- DFA combination state space
- Edge constraint BDD structure
- Backward search propagation
- Simplified state status flow
- Edge exploration loop detail
- DFS stack behavior
- ADD usage in DFA combination

**[EDGE_CONSTRAINT_SPEC.md](system_architecture/EDGE_CONSTRAINT_SPEC.md)** - Edge Constraint Mechanism
- EdgeCons data structure and building process
- Winning condition check with formal definitions
- BDD/ADD variable representation
- Signal processing, example walkthrough
- Complexity analysis

**[SUMMARY.md](system_architecture/SUMMARY.md)** - Executive Summary
- Overview of all documentation
- Key simplifications made
- Core functionality preserved

---

## Reimplementation Planning (`reimplementation/`)

**[NEW_REPOSITORY_SPEC.md](reimplementation/NEW_REPOSITORY_SPEC.md)** - Rust Reimplementation Plan
- Repository structure
- Core data structures in Rust
- 12-week implementation timeline
- Technology stack with Rust dependencies

---

## Quick Reference

### For Formula Module Redesign

1. **Start with**: [formula_module/CORE_FUNCTIONALITY.md](formula_module/CORE_FUNCTIONALITY.md) - What to keep
2. **Memory management**: [formula_module/MEMORY_MANAGEMENT_DECISION.md](formula_module/MEMORY_MANAGEMENT_DECISION.md) - ⭐ FINAL DECISION
3. **Variable handling**: [formula_module/ATOMIC_VARIABLE_HANDLING.md](formula_module/ATOMIC_VARIABLE_HANDLING.md) - ⭐ FINAL DECISION
4. **Then read**: [formula_module/PROPOSAL_B_ARCHITECTURE.md](formula_module/PROPOSAL_B_ARCHITECTURE.md) - How to design
5. **Reference**: [formula_module/ADVANCED_OPTIMIZATION_TECHNIQUES.md](formula_module/ADVANCED_OPTIMIZATION_TECHNIQUES.md) - Advanced concepts
6. **Compare**: [formula_module/REDESIGN_PROPOSALS.md](formula_module/REDESIGN_PROPOSALS.md) - All options

### For Understanding System Architecture

1. **Start with**: [system_architecture/SUMMARY.md](system_architecture/SUMMARY.md) - High-level overview
2. **Then**: [system_architecture/COMB_FULL_1_COMP_IDX_1_ANALYSIS.md](system_architecture/COMB_FULL_1_COMP_IDX_1_ANALYSIS.md) - Detailed analysis
3. **Visualize**: [system_architecture/FLOWCHART.md](system_architecture/FLOWCHART.md) - Flowcharts
4. **Deep dive**: [system_architecture/EDGE_CONSTRAINT_SPEC.md](system_architecture/EDGE_CONSTRAINT_SPEC.md) - Specific mechanism

### For Theoretical Foundations

1. **LTLf basics**: [formula_module/LTLf_BASIC.md](formula_module/LTLf_BASIC.md)
2. **Formula operations**: [formula_module/FORMULA_OPERATIONS.md](formula_module/FORMULA_OPERATIONS.md)
3. **Implementation details**: [formula_module/SIMPLIFY_IMPLEMENTATION.md](formula_module/SIMPLIFY_IMPLEMENTATION.md)

---

## Document Statistics

| Category | Documents | Lines (approx.) |
|----------|-----------|-----------------|
| **Formula Module** | 9 | ~7,500 |
| **Formula Redesign** | 4 | ~3,000 |
| **System Architecture** | 4 | ~2,000 |
| **Reimplementation** | 1 | ~400 |
| **Total** | 18 | ~12,900 |

---

## Design Decisions and Constraints

### User Constraints (from redesign discussions)

1. **Multi-threading**: Not a concern - single-threaded optimization only
   - Remove thread-safety considerations from proposals
   - Simplify cache design (no atomic operations needed)

2. **Future BDD Replacement**: Current deduplication approach may be replaced with BDD
   - Keep good abstraction to facilitate future BDD replacement
   - But don't over-abstract - BDD optimization is a major change
   - API incompatibility is acceptable when migrating to BDD
   - **Variable pre-allocation** design chosen for BDD compatibility

3. **Design Priority**: Clean architecture over micro-optimizations
   - Prefer simple, maintainable code
   - Only use advanced techniques (pointer packing, etc.) if clearly beneficial
   - **Arena allocators and pointer packing excluded** from implementation

4. **Single DFA Context**: Only one DFA construction at a time
   - Per-DFA resource management (FormulaPool, variable space)
   - No need for concurrent formula processing
   - Sub-formula sharing is the priority

---

## Dependencies Between Documents

```
LTLf_BASIC.md (theory)
    ↓
FORMULA_OPERATIONS.md (theory → implementation)
    ↓
SIMPLIFY_IMPLEMENTATION.md (implementation analysis)
    ↓
CORE_FUNCTIONALITY.md (extract essential parts)
    ↓
REDESIGN_PROPOSALS.md (proposal options)
    ↓
PROPOSAL_B_ARCHITECTURE.md (detailed design) ← ADVANCED_OPTIMIZATION_TECHNIQUES.md
```

---

## Last Updated

2025-01-01 (based on git commits)

**Note**: This index should be updated as new documents are added or existing documents are modified.
