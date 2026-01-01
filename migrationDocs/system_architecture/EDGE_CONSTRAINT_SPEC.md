# Edge Constraint Building Mechanism

## Overview

Edge constraints are the core data structure that encodes the game graph as BDDs (Binary Decision Diagrams). They represent:
- **Successors**: Which states can be reached from the current state
- **Input constraint (afX)**: What input assignments are possible
- **Output constraint (afY)**: What output assignments guarantee transitions to specific successors

## Core Data Structure

```cpp
class EdgeCons {
public:
    std::unordered_set<u_int64_t> succIdSet_;  // Successor state IDs
    ADD searched_afX_;                          // Input constraint (ADD)
    ADD searched_afY_;                          // Output constraint (ADD)
    Status status_;                             // Current status

    // Methods
    Status checkSwin4BackwardSearch(const Syn_Status_Cache &status_cache);
    void processSignal_Succ(Signal sig, u_int64_t succ_hashId);
};
```

## Building Process

### 1. Individual DFA Edge Constraints

**File:** `lib/include/ltlfsyn/edge_cons_builder.cpp`

```
Input: Syn_Frame (game state) with formula f
Output: EdgeCons with successors and constraints

Process:
│
├─ Step 1: Compute all possible successors
│  ├─ For each X (input assignment):
│  │  ├─ For each Y (output assignment):
│  │  │  ├─ Compute next_state = rmnext(f, X ∧ Y)
│  │  │  └─ Collect unique next_states
│  │
│  └─ succIdSet = {hashId(s) | s ∈ next_states}
│
├─ Step 2: Build afX (Input Constraint)
│  └─ afX = ⋁_{s∈succIdSet} (input_formula_leading_to_s)
│     └─ BDD over input variables X
│
├─ Step 3: Build afY (Output Constraint)
│  └─ afY = ⋁_{s∈succIdSet} (X ∧ Y → transition_to_s)
│     └─ BDD over X ∪ Y variables
│
└─ Step 4: Determine Initial Status
   ├─ If all successors are Swin: mark as Swin
   ├─ If ∃X such that all transitions lead to Ewin: mark as Ewin
   └─ Otherwise: Dfs_complete
```

### 2. Combined DFA Edge Constraints

**File:** `lib/include/dfa_combine/dfa_comb_tarjan.cpp`

For combined state (q1, q2):

```
Process:
│
├─ Step 1: Get successors from component DFAs
│  ├─ succ1 = {s1 | q1 → s1 in DFA1}
│  ├─ succ2 = {s2 | q2 → s2 in DFA2}
│  └─ Combined successors = succ1 × succ2
│
├─ Step 2: Build combined edge constraints
│  ├─ afX = afX1 ∧ afX2
│  └─ afY = afY1 ∧ afY2
│
└─ Step 3: Create combined successor states
   └─ For each (s1, s2):
       create DfaComb_Frame(s1, s2)
```

## Winning Condition Check

The core logic for determining if a state is winning:

```cpp
Status EdgeCons::checkSwin4BackwardSearch(const Syn_Status_Cache &status_cache) {
    // Check if all successors are winning
    for (u_int64_t succId : succIdSet_) {
        if (!status_cache.isSwin(succId)) {
            return Status::Ewin;  // Found a losing successor
        }
    }

    // Check if system can force to winning successors
    ADD winning_Y = computeWinningOutputAssignments(status_cache);

    if (winning_Y covers all inputs) {
        return Status::Swin;  // Can force to winning states
    }

    return Status::Ewin;  // Cannot guarantee winning
}
```

### Formal Definition

Given state q with successors {q1, q2, ..., qn}:

**System Winning (Swin)**:
```
∀X ∈ Inputs(X), ∃Y ∈ Outputs(Y), succ(q, X, Y) ⊆ Swin
```
- For all input assignments, there exists an output assignment such that all successors are winning.

**Environment Winning (Ewin)**:
```
∃X ∈ Inputs(X), ∀Y ∈ Outputs(Y), ∃q' ∈ succ(q, X, Y), q' ∈ Ewin
```
- There exists an input assignment such that for all output assignments, some successor is losing.

## BDD/ADD Variable Representation

```
Variables: [X1, X2, ..., Xn, Y1, Y2, ..., Ym]
           ↑         ↑        ↑
        Environment    |    System
                       |
                All variables

ADD (Algebraic Decision Diagram) represents:
- Input constraints: which (input) assignments are valid
- Transition mappings: (input, output) → successor state ID
```

## Signal Processing

When a successor's status changes, propagate signals backward:

```cpp
void EdgeCons::processSignal_Succ(Signal sig, u_int64_t succ_hashId) {
    switch (sig) {
        case Signal::To_swin:
            // A successor became winning
            // Update winning output assignments
            // May upgrade current state to Swin
            break;

        case Signal::To_ewin:
            // A successor became losing
            // May downgrade current state to Ewin
            // If all successors are Ewin, mark as Ewin
            break;
    }
}
```

## Example Walkthrough

### Input
```
State q with formula: F(p → X q)
Variables: X = {env}, Y = {p}
```

### Step 1: Compute Successors
```
For env=true, p=true:
  next_state = rmnext(F(p → X q), true ∧ true)
             = q

For env=true, p=false:
  next_state = rmnext(F(p → X q), true ∧ false)
             = F(X q)  (different state)

For env=false, p=true:
  next_state = rmnext(F(p → X q), false ∧ true)
             = q

For env=false, p=false:
  next_state = rmnext(F(p → X q), false ∧ false)
             = q

succIdSet = {hashId(q), hashId(F(X q))}
```

### Step 2: Build Constraints
```
afX = true  (all input values possible)

afY = (env ∧ p ∧ q) ∨ (env ∧ ¬p ∧ F(X q)) ∨
     (¬env ∧ p ∧ q) ∨ (¬env ∧ ¬p ∧ q)
```

### Step 3: Determine Status
```
If both q and F(X q) are Swin:
  → Current state is Swin

If F(X q) is Ewin and all transitions lead there:
  → Current state is Ewin

Otherwise:
  → Dfs_complete (need more info)
```

## Integration with Tarjan Algorithm

```
┌─────────────────────────────────────────────────────────┐
│              Tarjan DFS Loop                           │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  For each node:                                         │
│  ├─ visitNode()                                         │
│  │  └─ EdgeConsBuilder::build()                         │
│  │     ├─ Compute successors                           │
│  │     ├─ Build afX and afY                            │
│  │     └─ Set initial status                           │
│  │                                                      │
│  ├─ Explore successors via make_trans()                │
│  │                                                      │
│  └─ After SCC found:                                   │
│     └─ processScc()                                    │
│        └─ For each node:                               │
│           └─ EdgeCons::checkSwin4BackwardSearch()      │
│              └─ Update status based on successors       │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

## Key Files Reference

| File | Purpose |
|------|---------|
| `lib/include/ltlfsyn/edge_cons_builder.cpp` | Build edge constraints for individual DFAs |
| `lib/include/dfa_combine/dfa_comb_tarjan.cpp` | Build edge constraints for combined DFAs |
| `lib/include/synutil/trans_in_add.h` | BDD/ADD operations for transitions |
| `lib/include/synutil/formula_in_bdd.h` | Formula to BDD conversion |

## Complexity Analysis

### Time Complexity
- **Building Edge Constraints**: O(2^|X| × 2^|Y| × succ_count)
- **Swin Check**: O(BDD_size)
- **Signal Processing**: O(BDD_size × succ_count)

### Space Complexity
- **BDD Storage**: O(2^(|X|+|Y|)) in worst case
- **Successor Set**: O(succ_count)

Where:
- |X| = number of input variables
- |Y| = number of output variables
- succ_count = number of successor states
