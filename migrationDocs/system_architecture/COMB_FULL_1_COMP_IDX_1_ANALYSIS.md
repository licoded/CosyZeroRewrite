# Cosy Logic Extraction: COMB_FULL=1, USE_MINIMIZE=0, comp_idx=1

## Configuration Parameters

- **COMB_FULL=1**: Full combination mode - always combine all DFAs completely
- **USE_MINIMIZE=0**: Disable DFA minimization - work with full DFAs
- **comp_idx=1**: Use Incremental Composition strategy (`compositional_synthesis1`)

## High-Level Overview

```
Input: LTLf Formula + Variable Partition (inputs/outputs)
Output: Realizable/Unrealizable + Strategy (if realizable)

Main Flow:
1. Parse and simplify LTLf formula
2. Split into AND sub-formulas (if any)
3. For each sub-formula:
   a. Build individual DFA and solve synthesis game
   b. Incrementally combine with previous result
4. Return final realizability result
```

## Detailed Execution Flow

### Phase 1: Entry Point and Input Processing

**File:** `app/src/main.cpp:19-81`

```cpp
// Entry point
int main(int argc, const char *argv[]) {
    // 1. Read LTLf formula from file
    string af_s = read_ltlf_file(argv[1]);

    // 2. Read configuration flags
    aalta::COMB_FULL_FLAG = 1;        // Full combination mode
    aalta::USE_MINIMIZE_FLAG = 0;     // No minimization
    comp_idx_arg_str = "1";           // Incremental composition

    // 3. Read variable partition
    read_part_file(argv[2], env_var_names, sys_var_names);

    // 4. Select synthesis function
    syn_func = dfa_combine::compositional_synthesis1;

    // 5. Execute synthesis
    syn_res_flag = syn_func(af_s, env_var_names);
}
```

### Phase 2: Formula Parsing and Splitting

**File:** `lib/include/dfa_combine/dfs_product.cpp:116-131`

```cpp
bool compositional_synthesis1(af_str, env_var_names) {
    // Note: COMB_FULL=1 skips preprocessing checks

    // 1. Parse and simplify formula
    af = aalta_formula(af_str).nnf()->simplify()->nnf();

    // 2. Split into AND sub-formulas
    vector<aalta_formula *> and_sub_afs = getAndSubAfs(af);
    // Example: (G F p) & (G F q) -> [G F p, G F q]
}
```

### Phase 3: Incremental Composition Loop

**File:** `lib/include/dfa_combine/dfs_product.cpp:134-183`

For each sub-formula `i` from 0 to n-1:

```
Iteration i:
┌─────────────────────────────────────────────────────────┐
│ STEP 1: Individual Synthesis for sub_af[i]             │
├─────────────────────────────────────────────────────────┤
│ 1.1 Create WholeDFA_TarjanStrategy for sub_af[i]       │
│ 1.2 Execute Tarjan-based synthesis game solving         │
│ 1.3 If unrealizable -> return false                    │
│ 1.4 Continue without minimization (USE_MINIMIZE=0)      │
└─────────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────┐
│ STEP 2: Incremental Combination                        │
├─────────────────────────────────────────────────────────┤
│ 2.a If first formula (i==0):                           │
│     - Store exporter, continue to next formula         │
│                                                         │
│ 2.b For subsequent formulas:                          │
│     - Create combined formula:                         │
│       combined_af = AND(toNow_af, sub_af[i])           │
│     - Create exporters for both DFAs                   │
│     - Create WholeDFAComb_TarjanStrategy               │
│     - Execute combined synthesis                       │
│     - Update toNow_exporter with combined result       │
└─────────────────────────────────────────────────────────┘
```

### Phase 4: Individual DFA Synthesis (WholeDFA_TarjanStrategy)

**File:** `lib/include/ltlfsyn/syn_tarjan.cpp`

The `WholeDFA_TarjanStrategy` performs synthesis for a single formula:

```
WholeDFA_TarjanStrategy::doSynthesis()
│
├─ Initialize:
│  ├─ Create start node from formula
│  ├─ Initialize BDD manager for edge constraints
│  ├─ Initialize Tarjan algorithm structures
│
├─ Execute Tarjan DFS:
│  └─ Tarjan::dfsSearch(start_node)
│     │
│     ├─ For each node during DFS:
│     │  ├─ visitNode(node):
│     │  │  ├─ Check if accepting by empty trace
│     │  │  ├─ Build edge constraints using BDDs
│     │  │  └─ Record node in hashId2node map
│     │  │
│     │  ├─ getNextEdge(node): Get next outgoing edge
│     │  ├─ make_trans(node, edge): Compute successor state
│     │  │
│     │  └─ When SCC found:
│     │     └─ processScc(scc):
│     │        ├─ backwardSearch(scc): Propagate winning/losing
│     │        └─ Mark all nodes in SCC with final status
│     │
│     └─ Return: getStartNode status == Swin
│
└─ Return realizability result
```

### Phase 5: DFA Combination (WholeDFAComb_TarjanStrategy)

**File:** `lib/include/dfa_combine/dfa_comb_tarjan.cpp`

When combining two DFAs:

```
WholeDFAComb_TarjanStrategy Construction:
│
├─ Input:
│  ├─ state_af: AND(formula1, formula2)
│  ├─ env_var_names: Input variables
│  ├─ af1_syn_exporter: Exporter for first DFA
│  └─ af2_syn_exporter: Exporter for second DFA
│
├─ Initialize:
│  ├─ Create ID manager for combined states (DfaCombIdsMgr)
│  ├─ Initialize combined start node:
│  │    start = (af1_start, af2_start)
│  ├─ Initialize combined formula:
│  │    syn_af = AND(af1_formula, af2_formula)
│  └─ Import status caches from both DFAs
│
└─ Execute synthesis (similar to individual case)
```

**Combined State Representation:**

```cpp
// Combined node ID is a bit-pair:
// [dfa1_id | dfa2_id]
// - dfa1_id: State ID in first DFA
// - dfa2_id: State ID in second DFA
// - Bit allocation determined by dfa1_size

class DfaComb_Frame {
    u_int64_t dfa_comb_id;  // Combined ID
    std::vector<unsigned int> dfaIds;  // {dfa1_id, dfa2_id}

    // Get individual DFA IDs
    unsigned int getDfaId(int idx) { return dfaIds[idx]; }
};
```

### Phase 6: Tarjan SCC Algorithm

**File:** `lib/include/synutil/syn_tarjan.h:229-413`

The Tarjan algorithm finds strongly connected components (SCCs):

```
Tarjan DFS Algorithm:
│
├─ Data Structures:
│  ├─ dfn: Discovery time for each node
│  ├─ low: Lowest discovery time reachable
│  ├─ tarjan_sta: Stack for SCC detection
│  └─ dfs_sta: Stack for DFS traversal
│
└─ Algorithm (dfsSearch):
   │
   ├─ For node first visited:
   │  ├─ Set dfn[node] = low[node] = dfs_time++
   │  ├─ Push to both stacks
   │  └─ Call strategy->visitNode(node)
   │
   ├─ For each edge (node -> next):
   │  ├─ If next not visited:
   │  │  ├─ Recurse on next
   │  │  └─ Update: low[node] = min(low[node], low[next])
   │  │
   │  └─ If next visited and in stack:
   │     └─ Update: low[node] = min(low[node], dfn[next])
   │
   ├─ If dfn[node] == low[node] (SCC root found):
   │  ├─ Pop stack until node
   │  ├─ Collect nodes in SCC
   │  └─ Call strategy->processScc(scc)
   │
   └─ Pop node from DFS stack
```

### Phase 7: Edge Constraint Construction

**File:** `lib/include/ltlfsyn/edge_cons_builder.cpp`

Edge constraints encode the game as BDDs:

```
EdgeConsBuilder::build():
│
├─ For each node in DFA:
│  ├─ Compute edge_label:
│  │  ├─ X: Input variable assignments
│  │  └─ Y: Output variable assignments
│  │
│  ├─ For each successor state:
│  │  ├─ Build BDD for (X & Y -> successor)
│  │  └─ Add to edge constraint
│  │
│  └─ Determine initial status:
│     ├─ If all edges lead to Swin: Swin
│     ├─ If exists edge to Ewin with all X: Ewin
│     └─ Otherwise: Dfs_complete
│
└─ Return EdgeCons with succIdSet and BDDs
```

### Phase 8: Backward Search for SCC Resolution

**File:** `lib/include/ltlfsyn/syn_tarjan.cpp:125-191`

```
backwardSearch(scc):
│
├─ Initialize:
│  ├─ cur_swin: States marked as winning
│  ├─ undecided: States with Dfs_complete status
│  └─ predecessors: Reverse graph edges
│
├─ Iterate until no new winning states:
│  │
│  ├─ For each swin state:
│  │  ├─ Find all predecessors
│  │  └─ Add to candidate_new_swin
│  │
│  ├─ For each candidate:
│  │  ├─ Check if all successors lead to winning
│  │  ├─ If yes: mark as Swin
│  │  ├─ If all inputs lead to losing: mark as Ewin
│  │  └─ Add to new_swin
│  │
│  └─ cur_swin = new_swin
│
└─ Mark remaining undecided as Ewin
```

## Key Data Structures

### 1. Syn_Frame (Game State)
```cpp
class Syn_Frame {
    aalta_formula *state_af;     // Formula representing state
    EdgeCons *edge_cons;         // Edge constraints
    Status status;               // Swin/Ewin/Dfs_incomplete/Dfs_complete

    u_int64_t getHashId();       // Unique state identifier
};
```

### 2. EdgeCons (Game Edges)
```cpp
class EdgeCons {
    std::unordered_set<u_int64_t> succIdSet;  // Successor IDs
    ADD searched_afX;                          // Input constraint
    ADD searched_afY;                          // Output constraint

    Status checkSwin4BackwardSearch();         // Check if winning
};
```

### 3. DfaComb_Frame (Combined State)
```cpp
class DfaComb_Frame {
    u_int64_t dfa_comb_id;                     // Combined state ID
    std::vector<unsigned int> dfaIds;          // Component IDs

    // Access individual DFA states
    unsigned int getDfaId(int idx);
};
```

### 4. Status Cache
```cpp
class Syn_Status_Cache {
    std::vector<BDD> swin_state_bdd_vec;       // Winning states BDDs
    std::vector<BDD> ewin_state_bdd_vec;       // Losing states BDDs
    std::unordered_map<u_int64_t, Status> status_map;

    bool isSwin(u_int64_t hashId);
    bool isEwin(u_int64_t hashId);
    bool isEmptyAcc(u_int64_t hashId);
};
```

## State Transitions and Status

```
State Status Transitions:

┌────────────────────────────────────────────────────────┐
│                                                        │
│  [Unvisited]                                           │
│       │                                                │
│       ▼ visitNode()                                    │
│  [Dfs_incomplete] ──┐                                  │
│       │              │                                  │
│       ▼ make_trans() │                                 │
│  Explore successors  │                                  │
│       │              │                                  │
│       ▼              ▼                                  │
│  [Dfs_complete] ───► [Swin/Ewin] (if SCC resolved)      │
│       │                                                │
│       ▼ processScc() + backwardSearch()                │
│  [Swin] or [Ewin] (final status)                      │
│                                                        │
└────────────────────────────────────────────────────────┘

Status Values:
- Dfs_incomplete: Still exploring successors
- Dfs_complete: All successors explored, undecided
- Swin: System winning state
- Ewin: Environment winning state
```

## Winning Condition Check

```
Realizability Determination:

1. For individual DFA:
   ├─ Start node status == Swin → Realizable
   └─ Start node status == Ewin → Unrealizable

2. For combined DFA:
   ├─ Combined start node status == Swin → Realizable
   └─ Combined start node status == Ewin → Unrealizable

3. Winning Condition:
   ├─ System winning (Swin): ∃ strategy such that ∀ environment traces
   │  └─ All accepting states reachable
   │
   └─ Environment winning (Ewin): ∃ environment strategy such that
      └─ Can force system to non-accepting states
```

## File Structure Reference

```
lib/include/
├── dfa_combine/
│   ├── dfs_product.h/cpp          # Entry: compositional_synthesis1()
│   ├── dfa_comb_tarjan.h          # WholeDFAComb_TarjanStrategy
│   ├── dfa_comb_tarjan.cpp        # Combination logic
│   ├── dfa_comb_frame.h           # DfaComb_Frame (combined states)
│   └── edge_cons_builder.h         # Edge constraint construction
│
├── ltlfsyn/
│   ├── syn_tarjan.h/cpp           # WholeDFA_TarjanStrategy
│   ├── syn_frame.h                # Syn_Frame (game states)
│   └── synthesis.h                # isRealizable() wrapper
│
├── synutil/
│   ├── syn_tarjan.h/cpp           # Base TarjanStrategy
│   ├── tarjan_frame.h             # ITarjanFrame interface
│   ├── syn_status_cache.h         # Status management
│   ├── trans_in_add.h             # BDD for transitions
│   └── part_var.h                 # Variable partitioning
│
└── formula/
    ├── aalta_formula.h            # Formula representation
    └── af_utils.h                 # Formula utilities

app/src/
└── main.cpp                       # Entry point
```

## Important Considerations

### For COMB_FULL=1
1. **No Early Termination**: Always builds full combined DFA
2. **Complete Exploration**: All states explored regardless of early winning detection
3. **Memory Usage**: Higher memory usage due to complete state-space construction
4. **Correctness**: Guarantees correctness by exhaustive exploration

### For USE_MINIMIZE=0
1. **No DFA Minimization**: Works with full DFA from automata construction
2. **Larger State Space**: More states to process during synthesis
3. **No Pruning**: All states kept, including unreachable ones
4. **Performance**: Slower but preserves all information

## Summary of Key Functions

| Function | File | Purpose |
|----------|------|---------|
| `main()` | app/src/main.cpp | Entry point, arg parsing |
| `compositional_synthesis1()` | dfa_combine/dfs_product.cpp | Incremental composition |
| `WholeDFA_TarjanStrategy::doSynthesis()` | ltlfsyn/syn_tarjan.cpp | Individual DFA synthesis |
| `WholeDFAComb_TarjanStrategy::doSynthesis()` | dfa_combine/dfa_comb_tarjan.cpp | Combined DFA synthesis |
| `Tarjan::dfsSearch()` | synutil/syn_tarjan.h | SCC-based game solving |
| `EdgeConsBuilder::build()` | ltlfsyn/edge_cons_builder.cpp | Build edge constraints |
| `backwardSearch()` | ltlfsyn/syn_tarjan.cpp | Resolve SCCs |
| `make_trans()` | ltlfsyn/syn_tarjan.cpp | Compute successor states |
