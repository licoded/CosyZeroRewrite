# On-the-Fly Synthesis Implementation Plan

**Based on**: arXiv:2408.07324 - Section 3 (Tableau) & Section 4 (Game Solving)

**Target**: CosyZeroRewrite synthesis module

---

## Current Implementation Status

| Component | File | Status | Notes |
|-----------|------|--------|-------|
| Formula class | `formula/formula.hpp` | ✓ Complete | Immutable, hash-consed |
| NNF conversion | `nnf.cpp` | ✓ Complete | O(n) transformation |
| DFA structure | `automata/dfa.hpp` | ✓ Basic | Existing implementation |
| Game solver | `synthesis/game_solver.hpp` | ✓ Basic | Tarjan SCC implemented |
| is_realizable | `synthesis/synthesis.hpp` | ✓ Basic | Works for simple formulas |

**Gap**: Current implementation builds complete DFA before solving. Need on-the-fly approach.

---

## Implementation Roadmap

### Phase 1: Tableau State Management (Foundation)

**Files**: New: `automata/tableau.hpp`, `automata/tableau.cpp`

#### Task 1.1: Tableau State Representation

```cpp
class TableauState {
private:
    // Set of subformulas (as Formula pointers)
    std::unordered_set<Formula*, FormulaHash, FormulaEqual> formulas_;

    // Cached hash for equality checking
    size_t hash_;

public:
    // Create initial state from NNF formula
    static TableauState* initial(Formula* phi, FormulaPool& pool);

    // Check local consistency (Tableau 1)
    bool is_locally_consistent() const;

    // Check if state is accepting (satisfied)
    bool is_accepting() const;

    // Expand to next state given assignment
    TableauState* next(const std::unordered_set<int>& assignment,
                       FormulaPool& pool) const;

    // Get old() and next() components (Tableau 2)
    std::vector<Formula*> old_formulas() const;
    std::vector<Formula*> next_formulas() const;

    // Hash and equality for deduplication
    size_t hash() const;
    bool operator==(const TableauState& other) const;
};
```

**Implementation Notes**:
- Use `std::unordered_set<Formula*>` for formula storage
- Cache hash value on construction
- Hash consing via `TableauStatePool` (similar to FormulaPool)

#### Task 1.2: Tableau State Pool

```cpp
class TableauStatePool {
private:
    std::unordered_set<TableauState> unique_table_;
    std::vector<std::unique_ptr<TableauState>> states_;

public:
    TableauState* get_or_create(const std::unordered_set<Formula*>& formulas);
    size_t size() const;
};
```

**Verification**:
```cpp
// Test: same formula set → same pointer
FormulaPool f_pool;
TableauStatePool t_pool;

Formula* a = f_pool.create_variable("a");
TableauState* s1 = t_pool.get_or_create({a});
TableauState* s2 = t_pool.get_or_create({a});
assert(s1 == s2);  // Hash consing works
```

---

### Phase 2: On-the-Fly DFA Construction

**Files**: Modify `automata/dfa.hpp`, `automata/dfa.cpp`

#### Task 2.1: Lazy DFA Interface

```cpp
class OnTheFlyDFA {
private:
    TableauStatePool state_pool_;
    TableauState* initial_state_;
    FormulaPool& formula_pool_;

    // Cached transitions: (state, assignment) → next_state
    std::unordered_map<
        std::pair<TableauState*, Assignment>,
        TableauState*
    > transition_cache_;

    // Expanded states (for SCC detection)
    std::unordered_set<TableauState*> expanded_states_;

public:
    OnTheFlyDFA(Formula* phi, FormulaPool& pool);

    // Get initial state
    TableauState* initial() const { return initial_state_; }

    // Get or compute successor
    TableauState* successor(TableauState* q,
                            const Assignment& assignment);

    // Check if state is accepting
    bool is_accepting(TableauState* q) const;

    // Mark state as expanded (for game solving)
    void mark_expanded(TableauState* q);

    // Get all expanded states
    const std::unordered_set<TableauState*>& expanded_states() const;
};
```

**Key Design**:
- States are created **only when `successor()` is called**
- Transitions are cached for reuse
- `expanded_states_` tracks states reachable in current game

#### Task 2.2: Assignment Generation

```cpp
class AssignmentGenerator {
private:
    std::vector<std::string> output_vars_;
    std::vector<std::string> input_vars_;
    int num_outputs_;
    int num_inputs_;

public:
    AssignmentGenerator(const std::vector<std::string>& outputs,
                        const std::vector<std::string>& inputs);

    // Generate all 2^n output assignments
    std::vector<Assignment> all_output_assignments() const;

    // Generate all 2^m input assignments
    std::vector<Assignment> all_input_assignments() const;

    // Convert bitset to variable assignment
    Assignment from_bitset(uint64_t bits, bool is_output) const;
};
```

**Optimization**: Use Gray code ordering for efficient iteration.

---

### Phase 3: On-the-Fly Game Solver

**Files**: Modify `synthesis/game_solver.hpp`, `synthesis/game_solver.cpp`

#### Task 3.1: Game State Representation

```cpp
enum class Player { System, Environment };

enum class StateClass {
    Unknown,
    Swin,   // System winning
    Ewin,   // Environment winning
    Draw    // Not applicable for LTLf (finite traces)
};

struct GameState {
    TableauState* dfa_state;
    Player player;

    bool operator==(const GameState& other) const {
        return dfa_state == other.dfa_state && player == other.player;
    }
};

// Hash for GameState
struct GameStateHash {
    size_t operator()(const GameState& s) const {
        return std::hash<TableauState*>()(s.dfa_state) ^
               (static_cast<size_t>(s.player) << 1);
    }
};
```

#### Task 3.2: On-the-Fly Solver

```cpp
class OnTheFlyGameSolver {
private:
    OnTheFlyDFA& dfa_;
    AssignmentGenerator& assignment_gen_;

    // Game state classification
    std::unordered_map<GameState, StateClass, GameStateHash> classification_;

    // Worklist for on-the-fly expansion
    std::vector<GameState> worklist_;

    // Visited states
    std::unordered_set<GameState, GameStateHash> visited_;

    // Successor cache
    std::unordered_map<
        GameState,
        std::vector<GameState>,
        GameStateHash
    > successors_;

public:
    OnTheFlyGameSolver(OnTheFlyDFA& dfa,
                       AssignmentGenerator& gen);

    // Main algorithm: check realizability
    bool is_realizable();

    // Extract winning strategy (if realizable)
    Strategy extract_strategy();

private:
    // Expand game state on-demand
    void expand_state(const GameState& state);

    // Try to classify state using SCC analysis
    std::optional<StateClass> try_classify(const GameState& state);

    // Run Tarjan on currently expanded subgraph
    std::vector<std::vector<GameState>> find_sccs();

    // Backward propagation from classified states
    void propagate_classification();

    // Get successors for a state
    const std::vector<GameState>& get_successors(const GameState& state);
};
```

#### Task 3.3: Main Algorithm (from paper, Algorithm 2)

```cpp
bool OnTheFlyGameSolver::is_realizable() {
    // Initial state: system's turn on initial DFA state
    GameState initial{dfa_.initial(), Player::System};
    worklist_.push_back(initial);

    while (!worklist_.empty()) {
        GameState state = worklist_.back();
        worklist_.pop_back();

        if (visited_.count(state)) {
            continue;
        }

        // Expand state (compute successors)
        expand_state(state);

        // Try SCC classification on current subgraph
        auto sccs = find_sccs();

        for (const auto& scc : sccs) {
            auto cls = try_classify(scc);

            if (cls) {
                // SCC classified - update all states in it
                for (const auto& s : scc) {
                    classification_[s] = *cls;
                }

                // Propagate backward
                propagate_classification();

                // Check if initial state is classified
                if (classification_.count(initial)) {
                    return classification_[initial] == StateClass::Swin;
                }
            }
        }

        // Add unclassified successors to worklist
        for (const auto& succ : successors_[state]) {
            if (!classification_.count(succ)) {
                worklist_.push_back(succ);
            }
        }
    }

    // Exhausted all states without classifying initial
    // (should not happen for proper LTLf formulas)
    return classification_[initial] == StateClass::Swin;
}
```

---

### Phase 4: SCC Classification

#### Task 4.1: Modified Tarjan for Game Graph

```cpp
std::vector<std::vector<GameState>>
OnTheFlyGameSolver::find_sccs() {
    std::vector<std::vector<GameState>> result;
    std::unordered_map<GameState, int, GameStateHash> indices;
    std::unordered_map<GameState, int, GameStateHash> lowlinks;
    std::unordered_map<GameState, bool, GameStateHash> on_stack;
    std::vector<GameState> stack;

    int index = 0;

    std::function<void(GameState)> strongconnect = [&](GameState v) {
        indices[v] = index;
        lowlinks[v] = index;
        index++;
        stack.push_back(v);
        on_stack[v] = true;

        for (const GameState& w : get_successors(v)) {
            if (!indices.count(w)) {
                strongconnect(w);
                lowlinks[v] = std::min(lowlinks[v], lowlinks[w]);
            } else if (on_stack[w]) {
                lowlinks[v] = std::min(lowlinks[v], indices[w]);
            }
        }

        if (lowlinks[v] == indices[v]) {
            std::vector<GameState> scc;
            GameState w;
            do {
                w = stack.back();
                stack.pop_back();
                on_stack[w] = false;
                scc.push_back(w);
            } while (w != v);
            result.push_back(scc);
        }
    };

    for (const auto& pair : successors_) {
        const GameState& v = pair.first;
        if (!indices.count(v)) {
            strongconnect(v);
        }
    }

    return result;
}
```

#### Task 4.2: SCC Classification Logic

```cpp
std::optional<StateClass>
OnTheFlyGameSolver::try_classify(const std::vector<GameState>& scc) {
    // Check if SCC is fully classified
    bool all_classified = true;
    for (const auto& state : scc) {
        if (!classification_.count(state)) {
            all_classified = false;
            break;
        }
    }
    if (all_classified) {
        return classification_[scc[0]];
    }

    // Check if SCC has accepting DFA state
    bool has_accepting = false;
    for (const auto& state : scc) {
        if (dfa_.is_accepting(state.dfa_state)) {
            has_accepting = true;
            break;
        }
    }

    // Classification rule from paper:
    // - If SCC has accepting state → System winning (Swin)
    // - If SCC has no accepting state → Environment winning (Ewin)
    if (has_accepting) {
        return StateClass::Swin;
    } else {
        return StateClass::Ewin;
    }
}
```

#### Task 4.3: Backward Propagation

```cpp
void OnTheFlyGameSolver::propagate_classification() {
    bool changed = true;
    while (changed) {
        changed = false;

        for (auto& [state, successors] : successors_) {
            if (classification_.count(state)) {
                continue;  // Already classified
            }

            if (state.player == Player::System) {
                // System wins if ANY successor is Swin
                bool has_swin = false;
                bool all_ewin = !successors.empty();
                for (const auto& succ : successors) {
                    if (classification_[succ] == StateClass::Swin) {
                        has_swin = true;
                        break;
                    }
                    if (classification_[succ] != StateClass::Ewin) {
                        all_ewin = false;
                    }
                }

                if (has_swin) {
                    classification_[state] = StateClass::Swin;
                    changed = true;
                } else if (all_ewin && !successors.empty()) {
                    classification_[state] = StateClass::Ewin;
                    changed = true;
                }
            } else {
                // Environment wins if ALL successors are Ewin
                bool all_ewin = !successors.empty();
                bool has_swin = false;
                for (const auto& succ : successors) {
                    if (classification_[succ] == StateClass::Ewin) {
                        has_swin = false;  // Continue checking
                    } else {
                        all_ewin = false;
                        if (classification_[succ] == StateClass::Swin) {
                            has_swin = true;
                        }
                    }
                }

                if (all_ewin) {
                    classification_[state] = StateClass::Ewin;
                    changed = true;
                } else if (has_swin) {
                    classification_[state] = StateClass::Swin;
                    changed = true;
                }
            }
        }
    }
}
```

---

### Phase 5: State Expansion

```cpp
void OnTheFlyGameSolver::expand_state(const GameState& state) {
    if (successors_.count(state)) {
        return;  // Already expanded
    }

    std::vector<GameState> succs;

    if (state.player == Player::System) {
        // System chooses output assignment
        auto output_assignments = assignment_gen_.all_output_assignments();
        for (const auto& out_assn : output_assignments) {
            succs.push_back({state.dfa_state, Player::Environment});
        }
    } else {
        // Environment chooses input assignment
        auto input_assignments = assignment_gen_.all_input_assignments();

        // For each input assignment, compute next DFA state
        for (const auto& in_assn : input_assignments) {
            // Combine with current output (from previous system move)
            Assignment combined = combine_assignments(state, in_assn);
            TableauState* next_dfa = dfa_.successor(state.dfa_state, combined);
            succs.push_back({next_dfa, Player::System});
        }
    }

    successors_[state] = succs;
}
```

**Note**: The actual implementation needs to track the chosen output assignment
through the game state. Use a more complete state representation:

```cpp
struct GameState {
    TableauState* dfa_state;
    Player player;
    Assignment current_output;  // Only valid when player == Environment

    bool operator==(const GameState& other) const;
};
```

---

### Phase 6: Testing

**Files**: `tests/on_the_fly_synthesis_tests.cpp`

#### Test Categories

1. **Tableau Construction Tests**
   - Initial state from single formula
   - Expansion with assignments
   - Local consistency checking
   - Accepting state detection

2. **On-the-Fly DFA Tests**
   - Lazy state expansion
   - Transition caching
   - State deduplication

3. **Game Solving Tests**
   - Simple formulas: `a`, `X a`, `a & b`
   - Temporal operators: `F a`, `G a`, `a U b`
   - Unrealizable formulas: `a & !a`, `G (a | !a)` with constraints

4. **Benchmark Comparison**
   - Compare results with existing `synthesis_tests`
   - Check state count (should be ≤ current implementation)

---

## Implementation Order

| Phase | Task | Priority | Est. Time |
|-------|------|----------|-----------|
| 1 | Tableau State | P0 | 4 hours |
| 1 | Tableau Pool | P0 | 2 hours |
| 2 | Lazy DFA | P0 | 6 hours |
| 2 | Assignment Generator | P0 | 2 hours |
| 3 | Game State | P0 | 1 hour |
| 3 | Main Algorithm | P0 | 8 hours |
| 4 | Tarjan + SCC | P0 | 4 hours |
| 4 | Classification | P0 | 3 hours |
| 4 | Backward Propagation | P0 | 3 hours |
| 5 | State Expansion | P0 | 4 hours |
| 6 | Tests | P1 | 6 hours |

**Total**: ~43 hours (~1 week focused work)

---

## Key Design Decisions

### 1. Hash Consing for Tableau States

Same as Formula: use `unordered_set` with cached hashes. Ensures structural uniqueness.

### 2. Assignment Representation

Use `std::unordered_set<int>` (set of positive variable IDs for true values).
Empty set = all false.

### 3. Worklist vs. Recursion

Iterative worklist avoids stack overflow for large formulas.

### 4. Early Termination

Check if initial state is classified after each SCC round. Early exit on success.

---

## Integration with Existing Code

```cpp
// New synthesis function using on-the-fly solver
bool is_realizable_on_the_fly(Formula* phi, FormulaPool& pool) {
    // 1. Convert to NNF
    Formula* nnf_phi = phi->nnf(pool);

    // 2. Create tableau DFA
    OnTheFlyDFA dfa(nnf_phi, pool);

    // 3. Create assignment generator
    AssignmentGenerator gen(pool.get_outputs(), pool.get_inputs());

    // 4. Run solver
    OnTheFlyGameSolver solver(dfa, gen);
    return solver.is_realizable();
}
```

---

## References

- **Paper**: [arXiv:2408.07324](https://arxiv.org/abs/2408.07324) - Section 3 (Tableau), Section 4 (Game Solving)
- **Algorithm 1**: Tableau expansion (Tableau 1, 2)
- **Algorithm 2**: On-the-fly synthesis (Section 4.2)
