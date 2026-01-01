# New Repository Architecture Specification

## Project Goal

Create a clean, focused reimplementation of the Cosy LTLf synthesizer for the specific configuration:
- **COMB_FULL=1**: Full combination mode
- **USE_MINIMIZE=0**: No DFA minimization
- **comp_idx=1**: Incremental composition

## Implementation Language

### **Primary Language: C++** ⭐

**Decision**: Continue using C++ instead of Rust

**Rationale**:
- **Existing expertise**: Team has extensive C++ experience
- **Direct reuse**: Can leverage existing CUDD integration and BDD operations
- **Proven technology**: Current implementation is production-tested
- **Easier migration**: Can reference original implementation more directly
- **Performance**: C++ provides comparable performance to Rust for this use case

**C++ Standard**: C++20 or later

**Build System**: CMake (existing build infrastructure)

## Repository Structure

```
cosy_rewrite/
├── CMakeLists.txt                   # Main CMake configuration
├── README.md                        # Project overview
├── CLAUDE.md                        # Claude Code operating instructions
│
├── include/                         # Public headers
│   ├── formula/                     # Formula module interface
│   │   ├── formula.hpp              # Formula class
│   │   ├── formula_pool.hpp         # FormulaPool class
│   │   └── formula_ops.hpp          # Formula operations (nnf, simplify, etc.)
│   │
│   ├── parser/                      # Parser module interface
│   │   ├── ltlf_parser.hpp          # LTLf formula parser
│   │   └── partition_parser.hpp     # Partition file parser
│   │
│   ├── automata/                    # Automata interface
│   │   └── dfa.hpp                  # DFA data structure
│   │
│   ├── synthesis/                   # Synthesis interface
│   │   ├── synthesizer.hpp          # Synthesizer interface
│   │   └── incremental.hpp          # Incremental composition
│   │
│   ├── game/                        # Game solving interface
│   │   ├── game_graph.hpp           # Game graph structure
│   │   └── tarjan_scc.hpp           # Tarjan SCC algorithm
│   │
│   └── bdd/                         # BDD operations interface
│       └── edge_constraint.hpp      # Edge constraint BDDs
│
├── src/                             # Implementation
│   ├── formula/                     # Formula module implementation
│   │   ├── formula.cpp
│   │   ├── formula_pool.cpp
│   │   ├── nnf.cpp                  # NNF transformation
│   │   ├── xnf.cpp                  # XNF transformation
│   │   ├── simplify.cpp             # Simplification
│   │   └── rmnext.cpp               # Formula progression
│   │
│   ├── parser/                      # Parser implementation
│   │   ├── ltlf_parser.cpp
│   │   └── partition_parser.cpp
│   │
│   ├── automata/
│   │   └── dfa.cpp
│   │
│   ├── synthesis/
│   │   ├── synthesizer.cpp
│   │   └── incremental.cpp
│   │
│   ├── game/
│   │   ├── game_graph.cpp
│   │   └── tarjan_scc.cpp
│   │
│   ├── bdd/
│   │   └── edge_constraint.cpp
│   │
│   └── main.cpp                     # Entry point
│
├── tests/                           # Test suites
│   ├── formula_tests.cpp            # Formula module tests
│   ├── parser_tests.cpp             # Parser tests
│   ├── synthesis_tests.cpp          # Synthesis tests
│   └── integration_tests.cpp        # End-to-end tests
│
├── benchmarks/                      # Benchmark suites
│   └── synthesis_bench.cpp
│
└── docs/                            # Documentation (already exists)
    ├── formula_module/              # Formula module design docs
    ├── system_architecture/         # System architecture docs
    └── reimplementation/            # This document
```

## Core Data Structures

### 1. Formula (Immutable, Hash-Consed)

```cpp
namespace formula {

class FormulaPool;  // Forward declaration

class Formula {
public:
    enum class OpType {
        True, False,
        Not, And, Or,
        Next, Until, Release,
        Literal  // Variable reference
    };

    // Immutable accessors
    OpType op() const { return op_; }
    Formula* left() const { return left_; }
    Formula* right() const { return right_; }
    int var_id() const { return var_id_; }

    // Operations (return new formulas, never modify this)
    Formula* nnf() const;
    Formula* simplify() const;
    Formula* xnf_with_tail() const;
    Formula* rmnext(Formula* edge, const std::unordered_set<int>& all_vars) const;

    // String representation
    std::string to_string() const;

private:
    Formula(OpType op, Formula* left, Formula* right, int var_id);

    OpType op_;
    Formula* left_;
    Formula* right_;
    int var_id_;      // Only used when op_ == OpType::Literal
    size_t hash_;     // Cached hash value

    friend class FormulaPool;
};

} // namespace formula
```

### 2. FormulaPool (Per-DFA Resource Manager)

```cpp
namespace formula {

class FormulaPool {
public:
    FormulaPool();
    ~FormulaPool();

    // Variable management
    void load_from_partition(const std::string& partition_file);
    void declare_variables(const std::vector<std::string>& outputs,
                          const std::vector<std::string>& inputs);
    void declare_outputs(const std::vector<std::string>& outputs);
    void declare_inputs(const std::vector<std::string>& inputs);
    void extract_variables_from_formula(Formula* root);

    bool is_fully_declared() const;

    // Formula creation (canonicalized)
    Formula* create(OpType op, Formula* left, Formula* right, int var_id);
    Formula* create_variable(const std::string& name);
    Formula* create_true();
    Formula* create_false();

    // Variable info
    int get_variable_id(const std::string& name) const;
    std::string get_variable_name(int var_id) const;
    bool is_output_variable(int var_id) const;
    bool is_input_variable(int var_id) const;

    // Resource management
    void clear();  // Free all formulas

private:
    // Canonicalization cache
    std::unordered_set<Formula*, FormulaHash, FormulaEqual> unique_table_;

    // Variable management
    std::vector<std::string> var_names_;
    std::unordered_map<std::string, int> var_ids_;
    int num_outputs_;
    int num_inputs_;
    bool outputs_declared_;
    bool inputs_declared_;

    // Formula ownership
    std::vector<std::unique_ptr<Formula>> formulas_;
};

} // namespace formula
```

### 3. DFA State

```cpp
namespace automata {

using StateId = uint32_t;

class DfaState {
public:
    StateId id() const { return id_; }
    bool accepting() const { return accepting_; }
    const formula::Formula* formula() const { return formula_; }

private:
    StateId id_;
    bool accepting_;
    const formula::Formula* formula_;  // Reference, not owned
};

class Dfa {
public:
    StateId initial_state() const { return initial_; }
    const DfaState* get_state(StateId id) const;

    void add_state(std::unique_ptr<DfaState> state);
    void set_transition(StateId from, const Assignment& assignment, StateId to);

private:
    std::vector<std::unique_ptr<DfaState>> states_;
    StateId initial_;
    std::unordered_map<std::pair<StateId, Assignment>, StateId> transitions_;
};

} // namespace automata
```

### 4. Game Graph Node

```cpp
namespace game {

enum class NodeStatus {
    Unvisited,
    DFSIncomplete,
    DFSComplete,
    SystemWinning,   // SWin
    EnvironmentWinning  // EWin
};

class GameNode {
public:
    using NodeId = uint64_t;

    NodeId id() const { return id_; }
    const formula::Formula* formula() const { return formula_; }
    NodeStatus status() const { return status_; }
    void set_status(NodeStatus status) { status_ = status; }

    const bdd::EdgeConstraint* edge_constraint() const {
        return edge_constraint_;
    }

private:
    NodeId id_;
    const formula::Formula* formula_;  // Reference, not owned
    NodeStatus status_;
    const bdd::EdgeConstraint* edge_constraint_;
};

} // namespace game
```

## Module Interfaces

### Formula Module

```cpp
namespace formula {

// Public API
Formula* parse_ltlf(const std::string& input, FormulaPool& pool);

// High-level transformations
Formula* transform_to_xnf(Formula* f);
Formula* simplify_formula(Formula* f);

} // namespace formula
```

### Parser Module

```cpp
namespace parser {

struct Partition {
    std::vector<std::string> inputs;   // Environment variables
    std::vector<std::string> outputs;  // System/agent variables
};

Partition parse_partition_file(const std::string& path);
Partition parse_partition_string(const std::string& content);

} // namespace parser
```

### Synthesis Module

```cpp
namespace synthesis {

enum class Result {
    Realizable,
    Unrealizable,
    Error
};

class Synthesizer {
public:
    virtual Result synthesize(
        const formula::Formula& spec,
        const parser::Partition& partition
    ) = 0;

    virtual ~Synthesizer() = default;
};

class IncrementalSynthesizer : public Synthesizer {
public:
    Result synthesize(
        const formula::Formula& spec,
        const parser::Partition& partition
    ) override;
};

} // namespace synthesis
```

## Technology Stack

### Core Requirements

```cmake
cmake_minimum_required(VERSION 3.20)
project(CosyRewrite VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(Cudd REQUIRED)  # BDD library
find_package(spdlog REQUIRED)  # Logging

# External dependencies (from original project)
add_subdirectory(lib/deps/cudd)
add_subdirectory(lib/deps/mona)
```

### Third-Party Libraries

1. **CUDD**: BDD operations (existing)
2. **MONA**: Finite automata (existing)
3. **spdlog**: Logging (existing)
4. **Google Test**: Testing framework (new)
5. **Google Benchmark**: Benchmarking (new)

### Build Configuration

```cmake
# Release build (optimized)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Debug build (with symbols)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Testing
enable_testing()
add_subdirectory(tests)
```

## Algorithm Reimplementation Plan

### Phase 1: Formula Module (Weeks 1-2)
- [ ] Implement Formula class (immutable)
- [ ] Implement FormulaPool with variable management
- [ ] Implement NNF transformation
- [ ] Implement XNF transformation
- [ ] Implement simplification (O(n) version)
- [ ] Implement rmnext (formula progression)
- [ ] Add unit tests

### Phase 2: Parser Module (Week 3)
- [ ] Implement LTLf parser
- [ ] Implement partition parser
- [ ] Add error handling
- [ ] Test with various formulas

### Phase 3: Automata Interface (Week 4)
- [ ] Implement DFA data structure
- [ ] Integrate with external LTLf-to-DFA tool
- [ ] Add state management
- [ ] Test DFA construction

### Phase 4: Game Solving (Weeks 5-6)
- [ ] Implement Tarjan SCC algorithm
- [ ] Implement backward search
- [ ] Implement status propagation
- [ ] Test game solving

### Phase 5: Synthesis (Weeks 7-8)
- [ ] Implement incremental composition
- [ ] Add result reporting
- [ ] Test synthesis

### Phase 6: Integration & Testing (Week 9)
- [ ] Comprehensive testing
- [ ] Benchmark comparison with original
- [ ] Documentation

## Key Differences from Original

### Simplifications
1. **No Preprocessing**: Removed Lydia preprocessing checks
2. **No Minimization**: Removed DFA minimization
3. **No Visualization**: Removed DOT export
4. **Fixed Configuration**: Always COMB_FULL=1, USE_MINIMIZE=0, comp_idx=1

### Improvements
1. **Immutable Formulas**: No mutable state, easier reasoning
2. **Modern C++**: C++20 features (concepts, ranges, etc.)
3. **Clear Ownership**: FormulaPool owns all formulas
4. **Type Safety**: Strong typing with enums
5. **Testing**: Google Test framework
6. **Documentation**: Inline comments + design docs

### Features to Keep
1. **Incremental Composition**: Core algorithm
2. **Tarjan SCC**: Efficient game solving
3. **BDD Constraints**: Compact representation (CUDD)
4. **Backward Search**: SCC resolution
5. **LTLf Semantics**: Strong Next for finite traces

## Testing Strategy

### Unit Tests
- Formula parsing and manipulation
- Formula transformations (NNF, XNF, simplify, rmnext)
- DFA operations
- Individual algorithm components

### Integration Tests
- End-to-end synthesis
- Composition of multiple formulas
- Edge cases

### Benchmarks
- Compare with original implementation
- Performance regression tests
- Memory usage profiling

## Success Criteria

| Metric | Target | Measurement |
|--------|--------|-------------|
| Correctness | Pass all test cases | Test suite |
| Performance | Within 1.5x of original | Benchmarks |
| Memory | ≤ original memory | Profiling |
| Code Quality | High test coverage | gcov |
| Maintainability | Clear, documented code | Code review |

## Open Questions

1. **External Tools**: Use existing MONA/Lydia integration?
   - **Recommendation**: Yes, reuse existing FFI

2. **Testing Framework**: Google Test or Catch2?
   - **Recommendation**: Google Test (industry standard)

3. **Logging**: Keep spdlog or switch?
   - **Recommendation**: Keep spdlog (already integrated)

4. **Formula Optimization**: How aggressive should O(n) simplification be?
   - **Recommendation**: Implement all optimizations from SIMPLIFY_IMPLEMENTATION.md

## Documentation Structure

- **README.md**: Quick start guide
- **ARCHITECTURE.md**: System design (this document)
- **docs/formula_module/**: Detailed formula module design (existing)
- **docs/system_architecture/**: Algorithm documentation (existing)
- **API.md**: Generated by Doxygen

## Migration from Original

### Code Reuse
1. **CUDD integration**: Direct reuse
2. **MONA integration**: Direct reuse
3. **Algorithm logic**: Reference implementation, rewrite with modern C++
4. **Test cases**: Port existing test suite

### Files to Remove
1. **aalta_formula.cpp/h**: Replace with new Formula/FormulaPool
2. **af_utils.cpp/h**: Replace with new formula operations
3. **Unused features**: Preprocessing, minimization, visualization

### Files to Keep
1. **CUDD wrapper**: Already working
2. **MONA wrapper**: Already working
3. **Main synthesis loop**: Reference for rewrite
