# Cosy Code Analysis and Reimplementation Plan

## Executive Summary

This document summarizes the analysis of the Cosy LTLf synthesizer codebase for the specific configuration **(COMB_FULL=1, USE_MINIMIZE=0, comp_idx=1)** and provides a plan for reimplementation in a new repository.

## Configuration Parameters

| Parameter | Value | Meaning |
|-----------|-------|---------|
| COMB_FULL | 1 | Full combination mode |
| USE_MINIMIZE | 0 | No DFA minimization |
| comp_idx | 1 | Incremental composition |

## Analysis Summary

### Code Flow Overview

```
Input LTLf Formula
         ↓
Parse and Simplify
         ↓
Split into AND Sub-formulas
         ↓
┌────────────────────────────┐
│ For Each Sub-formula:      │
│  1. Build Individual DFA   │
│  2. Solve Synthesis Game   │
│  3. Incrementally Combine  │
└────────────────────────────┘
         ↓
Return Realizability Result
```

### Key Components Identified

1. **Formula Processing**
   - LTLf formula parsing and simplification
   - NNF (Negation Normal Form) transformation
   - AND sub-formula decomposition

2. **Automata Construction**
   - LTLf to DFA conversion (via external tool)
   - DFA state management
   - Transition relation encoding

3. **Game Solving**
   - Tarjan SCC-based algorithm
   - Backward search for winning regions
   - BDD-based edge constraints

4. **Incremental Composition**
   - Progressive DFA combination
   - Product state space construction
   - Combined game solving

## Documentation Created

### 1. COMB_FULL_1_COMP_IDX_1_ANALYSIS.md
Comprehensive analysis covering:
- Entry points and main execution flow
- Incremental composition loop (simplified - no preprocessing)
- Individual DFA synthesis (WholeDFA_TarjanStrategy)
- DFA combination (WholeDFAComb_TarjanStrategy)
- Tarjan SCC algorithm
- Edge constraint construction
- Backward search for SCC resolution
- Key data structures and file structure reference

### 2. FLOWCHART.md
Visual documentation including:
- High-level system flow
- Detailed Tarjan algorithm flow
- DFA combination state space
- Edge constraint BDD structure
- Backward search propagation
- **Simplified** state status flow (preprocessing removed)
- Component interaction diagram

### 3. EDGE_CONSTRAINT_SPEC.md
Detailed specification of:
- Core data structure (EdgeCons)
- Building process for individual and combined DFAs
- Winning condition check
- BDD operations
- Signal processing
- Example walkthrough
- Integration with Tarjan algorithm
- Complexity analysis

### 4. NEW_REPOSITORY_SPEC.md
Complete reimplementation plan including:
- Rust-based architecture design
- Repository structure (simplified)
- Technology stack (removed debug dependencies)
- Core data structures
- Module interfaces
- 12-week implementation plan (simplified)
- Testing strategy
- Success criteria

## Key Algorithms

### 1. Tarjan SCC-Based Game Solving
```
For each state in DFS order:
  1. Compute discovery time (dfn)
  2. Track lowest reachable (low)
  3. When dfn == low: SCC found
  4. Process SCC: backward search to determine winning/losing
```

### 2. Incremental Composition
```
For each sub-formula f_i in [f_1, f_2, ..., f_n]:
  1. Build DFA_i and solve synthesis
  2. If i > 0:
     a. Combine: DFA_combined = DFA_1 × ... × DFA_i
     b. Solve synthesis on combined DFA
```

### 3. Backward Search
```
In SCC with states {s_1, ..., s_n}:
  1. Initialize: cur_swin = {states marked Swin}
  2. Repeat:
     a. Find predecessors of cur_swin
     b. Check if they can force to Swin
     c. Update statuses
  3. Mark undecided as Ewin
```

## Data Structures

| Structure | Purpose | Key Fields |
|-----------|---------|------------|
| Syn_Frame | Game state | formula, edge_cons, status |
| EdgeCons | Game edges | succIdSet, afX, afY |
| DfaComb_Frame | Combined state | dfa_comb_id, dfaIds |
| StatusCache | Status tracking | swin_vec, ewin_vec |

## Reimplementation Plan

### Technology Choice: Rust

**Advantages:**
- Memory safety without GC
- Performance comparable to C++
- Modern tooling (Cargo, rustdoc)
- Built-in testing
- Strong type system

### Project Structure

```
ltlf-synthesizer/
├── src/
│   ├── cli/         # Command-line interface
│   ├── parser/      # LTLf and partition parsing
│   ├── formula/     # Formula AST and transformations
│   ├── automata/    # DFA interface
│   ├── game/        # Game solving algorithms
│   ├── bdd/         # BDD operations
│   ├── synthesis/   # Synthesis algorithms
│   └── error/       # Error types
├── tests/           # Tests
├── examples/        # Usage examples
└── benches/         # Benchmarks
```

### Implementation Timeline

12 weeks total:
- Weeks 1-2: Core infrastructure
- Weeks 3-5: Parsing and automata
- Weeks 6-7: BDD operations
- Weeks 8-9: Game solving
- Weeks 10-11: Synthesis
- Week 12: Testing and documentation

## Next Steps

With this comprehensive analysis and planning, the project is ready to move to **creating a new repository**. The documentation provides everything needed to start the reimplementation with confidence.

## Files Created

```
docs/
├── COMB_FULL_1_COMP_IDX_1_ANALYSIS.md   # Complete code analysis (simplified)
├── FLOWCHART.md                          # Visual documentation (simplified)
├── EDGE_CONSTRAINT_SPEC.md               # Edge constraint details
├── NEW_REPOSITORY_SPEC.md                # Reimplementation plan (simplified)
└── SUMMARY.md                            # This summary
```

## Key Simplifications Made

From the original codebase, the following have been removed for the focused reimplementation:

1. **Preprocessing Checks**: `callLydiaPreprocess()`, `checkEverySubAf()`
2. **DFA Minimization**: All minimization-related code
3. **Debug/Visualization**: DOT export, graph visualization
4. **Early Termination**: On-the-fly optimization for last formula
5. **Signal Processing**: Status propagation optimization
6. **Configuration Flags**: Fixed to COMB_FULL=1, USE_MINIMIZE=0, comp_idx=1

## What Remains (Core Logic)

The following core components are preserved:

1. ✅ Formula parsing and NNF transformation
2. ✅ LTLf to DFA conversion (via external tool)
3. ✅ DFA data structures
4. ✅ Tarjan SCC-based game solving
5. ✅ BDD edge constraint construction
6. ✅ Backward search for SCC resolution
7. ✅ Incremental DFA combination
8. ✅ Status cache management

This focused approach will result in a cleaner, more maintainable codebase while preserving all essential functionality for the target configuration.
