# Cosy Execution Flowchart (COMB_FULL=1, USE_MINIMIZE=0, comp_idx=1)

## High-Level System Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                           INPUT STAGE                                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐          │
│  │  LTLf File   │    │  Part File   │    │ Configuration│          │
│  │              │    │              │    │              │          │
│  │  formula.str │    │  inputs/outs │    │comp_idx=1    │          │
│  │              │    │              │    │COMB_FULL=1   │          │
│  │              │    │              │    │USE_MINIMIZE=0│          │
│  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘          │
│         │                   │                   │                     │
│         ▼                   ▼                   │                     │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │        Formula Parsing & Simplification                      │    │
│  │  af = parse().nnf().simplify().nnf()                         │    │
│  └──────────────────────────────┬──────────────────────────────┘    │
│                                 │                                     │
│                                 ▼                                     │
│         ┌─────────────────────────────────┐                         │
│         │  Split into AND Sub-formulas    │                         │
│         │  getAndSubAfs(af)                │                         │
│         │  [af1, af2, af3, ...]            │                         │
│         └─────────────────────────────────┘                         │
└───────────────────────────────────┬─────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     INCREMENTAL COMPOSITION LOOP                     │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│   For each sub_af in [af1, af2, af3, ...]:                           │
│   │                                                                  │
│   │  ┌────────────────────────────────────────────────────────┐    │
│   │  │  STEP 1: Individual DFA Synthesis                       │    │
│   │  ├────────────────────────────────────────────────────────┤    │
│   │  │                                                         │    │
│   │  │  ┌──────────────────────────────────────────────────┐  │    │
│   │  │  │ Create WholeDFA_TarjanStrategy(sub_af)           │  │    │
│   │  │  └────────────────────┬─────────────────────────────┘  │    │
│   │  │                       ▼                                │    │
│   │  │  ┌──────────────────────────────────────────────────┐  │    │
│   │  │  │ Build DFA from LTLf formula                      │  │    │
│   │  │  │ (using external tool for automata construction)  │  │    │
│   │  │  └────────────────────┬─────────────────────────────┘  │    │
│   │  │                       ▼                                │    │
│   │  │  ┌──────────────────────────────────────────────────┐  │    │
│   │  │  │ Tarjan SCC-based Game Solving                    │  │    │
│   │  │  │ ├─ DFS traversal of state space                  │  │    │
│   │  │  │ ├─ SCC decomposition                             │  │    │
│   │  │  │ ├─ Backward search for winning regions           │  │    │
│   │  │  │ └─ Determine realizability                       │  │    │
│   │  │  └────────────────────┬─────────────────────────────┘  │    │
│   │  │                       ▼                                │    │
│   │  │  ┌──────────────────────────────────────────────────┐  │    │
│   │  │  │ Result: Realizable or Unrealizable?              │  │    │
│   │  │  └────────────────────┬─────────────────────────────┘  │    │
│   │  │                       │                                │    │
│   │  │             ┌─────────┴─────────┐                      │    │
│   │  │             ▼                   ▼                      │    │
│   │  │      [Unrealizable]        [Realizable]               │    │
│   │  │             │                   │                      │    │
│   │  │       return false        Continue (no minimization)  │    │
│   │  │                                                         │    │
│   │  └────────────────────────────────────────────────────────┘    │
│   │                       │ (if not first)                        │
│   │                       ▼                                        │
│   │  ┌────────────────────────────────────────────────────────┐    │
│   │  │  STEP 2: Incremental Combination                       │    │
│   │  ├────────────────────────────────────────────────────────┤    │
│   │  │                                                         │    │
│   │  │  ┌──────────────────────────────────────────────────┐  │    │
│   │  │  │ Create Combined Formula                           │  │    │
│   │  │  │ combined_af = AND(toNow_af, sub_af)               │  │    │
│   │  │  └────────────────────┬─────────────────────────────┘  │    │
│   │  │                       ▼                                │    │
│   │  │  ┌──────────────────────────────────────────────────┐  │    │
│   │  │  │ Create WholeDFAComb_TarjanStrategy               │  │    │
│   │  │  │ ├─ Import both DFAs                              │  │    │
│   │  │  │ ├─ Create combined state space                   │  │    │
│   │  │  │ └─ Initialize start node as (q1_start, q2)       │  │    │
│   │  │  └────────────────────┬─────────────────────────────┘  │    │
│   │  │                       ▼                                │    │
│   │  │  ┌──────────────────────────────────────────────────┐  │    │
│   │  │  │ Tarjan SCC on Combined DFA                       │  │    │
│   │  │  │ ├─ DFS on product state space                    │  │    │
│   │  │  │ ├─ SCC decomposition on combined states          │  │    │
│   │  │  │ ├─ Backward search                              │  │    │
│   │  │  │ └─ Determine realizability                       │  │    │
│   │  │  └────────────────────┬─────────────────────────────┘  │    │
│   │  │                       ▼                                │    │
│   │  │  ┌──────────────────────────────────────────────────┐  │    │
│   │  │  │ Combined Result                                  │  │    │
│   │  │  └────────────────────┬─────────────────────────────┘  │    │
│   │  │                       │                                │    │
│   │  │             ┌─────────┴─────────┐                      │    │
│   │  │             ▼                   ▼                      │    │
│   │  │      [Unrealizable]        [Realizable]               │    │
│   │  │             │                   │                      │    │
│   │  │       return false        Update toNow_exporter        │    │
│   │  │                                                         │    │
│   │  └────────────────────────────────────────────────────────┘    │
│   │                       │                                        │
│   └───────────────────────┼────────────────────────────────────────┘
│                           │
└───────────────────────────┼───────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│                           OUTPUT STAGE                               │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  Final Result:                                              │     │
│  │  ├─ Realizable/Unrealizable                                 │     │
│  │  ├─ Execution time                                          │     │
│  │  ├─ Node count statistics                                  │     │
│  │  └─ Memory usage                                           │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## Detailed Tarjan Algorithm Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│              TARJAN SCC-BASED GAME SOLVING                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  Initialize:                                                │     │
│  │  ├─ dfn = {}       (discovery times)                       │     │
│  │  ├─ low = {}       (lowest reachable)                       │     │
│  │  ├─ tarjan_stack = []                                       │     │
│  │  ├─ dfs_stack = []                                          │     │
│  │  └─ dfs_time = 0                                            │     │
│  └────────────────────────────┬─────────────────────────────────┘     │
│                               ▼                                      │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  Push start_node                                           │     │
│  └────────────────────────────┬─────────────────────────────────┘     │
│                               ▼                                      │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  WHILE dfs_stack not empty:                                │     │
│  │  ┌────────────────────────────────────────────────────┐    │     │
│  │  │  cur = dfs_stack.back()                             │    │     │
│  │  │                                                      │    │     │
│  │  │  IF shouldStopSearch(cur):                           │    │     │
│  │  │    ├─ IF isSccRoot(cur):                             │    │     │
│  │  │    │   ├─ Collect SCC from tarjan_stack              │    │     │
│  │  │    │   └─ processScc(scc)                            │    │     │
│  │  │    │                                                  │    │     │
│  │  │    │   processScc(scc):                              │    │     │
│  │  │    │   └─ backwardSearch(scc)                        │    │     │
│  │  │    │       └─ Mark nodes as Swin/Ewin                │    │     │
│  │  │    │                                                  │    │     │
│  │  │    └─ Pop from dfs_stack                             │    │     │
│  │  │                                                      │    │     │
│  │  │  ELSE:  # Continue exploration                        │    │     │
│  │  │    ├─ edge = getNextEdge(cur)                        │    │     │
│  │  │    ├─ next = make_trans(cur, edge)                   │    │     │
│  │  │    │                                                 │    │     │
│  │  │    ├─ IF next not visited:                           │    │     │
│  │  │    │  ├─ dfn[next] = low[next] = dfs_time++          │    │     │
│  │  │    │  ├─ Push to both stacks                         │    │     │
│  │  │    │  └─ visitNode(next)                             │    │     │
│  │  │    │                                                 │    │     │
│  │  │    └─ ELSE:  # Already visited                        │    │     │
│  │  │       └─ low[cur] = min(low[cur], dfn[next])         │    │     │
│  │  │                                                      │    │     │
│  │  └────────────────────────────────────────────────────┘    │     │
│  │                                                             │     │
│  └────────────────────────────────────────────────────────────┘     │
│                               ▼                                      │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  Return: status_cache[start_node]                           │     │
│  │    ├─ Swin → Realizable                                     │     │
│  │    └─ Ewin → Unrealizable                                   │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## DFA Combination State Space

```
┌─────────────────────────────────────────────────────────────────────┐
│                 COMBINED DFA STATE SPACE                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  DFA 1 States: Q1 = {q1_0, q1_1, q1_2, ..., q1_n}                   │
│  DFA 2 States: Q2 = {q2_0, q2_1, q2_2, ..., q2_m}                   │
│                                                                      │
│  Combined States: Q = Q1 × Q2                                       │
│                                                                      │
│  Example: q1_0 (start of DFA1) × q2_0 (start of DFA2)               │
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  DfaComb_Frame Representation:                              │     │
│  │  ├─ dfa_comb_id = (dfa1_id << offset) | dfa2_id           │     │
│  │  ├─ dfaIds = [dfa1_id, dfa2_id]                           │     │
│  │  └─ hashId = unique identifier                            │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
│  Transition Logic:                                                  │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  From state (q1, q2) with edge (x, y):                     │     │
│  │  ├─ Get DFA1 transition: q1' = δ1(q1, x, y)               │     │
│  │  ├─ Get DFA2 transition: q2' = δ2(q2, x, y)               │     │
│  │  └─ Combined successor: (q1', q2')                         │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
│  Accepting Condition:                                               │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  (q1, q2) is accepting iff:                                │     │
│  │  ├─ q1 is accepting in DFA1                               │     │
│  │  └─ q2 is accepting in DFA2                               │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### ADD Usage in DFA Combination

**ADD (Algebraic Decision Diagram)** is used to efficiently represent and compute combined edge constraints:

```
┌─────────────────────────────────────────────────────────────────────┐
│              ADD IN COMBINED EDGE CONSTRAINTS                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  TransInAddMgr addMgr_;  // Manages all ADD operations              │
│                                                                      │
│  For combined state (q1, q2):                                        │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  ADD searched_afX:                                           │     │
│  │  ├─ Represents input assignments leading to successors      │     │
│  │  └─ afX = afX1 ∧ afX2 (conjunction of component ADDs)      │     │
│  │                                                              │     │
│  │  ADD searched_afY:                                           │     │
│  │  ├─ Maps (input × output) → successor state ID             │     │
│  │  └─ afY = afY1 ∧ afY2 (conjunction of component ADDs)      │     │
│  │                                                              │     │
│  │  succIdSet:                                                   │     │
│  │  └─ HashSet of combined successor IDs                         │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
│  Key Operations:                                                     │
│  ├─ addMgr_.getAdd4Lit(lit_id): Get ADD for literal                │
│  ├─ addMgr_.transByEdgeAf(addP, edge): Evaluate ADD with edge      │
│  └─ addMgr_.createAddConstFunc(val, edge): Create ADD function     │
│                                                                      │
│  Why ADD instead of BDD?                                             │
│  ├─ ADD can represent integer values (state IDs) directly          │
│  ├─ ADD supports arithmetic operations for combination            │
│  └─ ADD enables efficient edge constraint conjunction             │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### Combination Process with ADD

```
┌─────────────────────────────────────────────────────────────────────┐
│         COMBINING TWO GAME STATES USING ADD                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Given: We want to combine two states from different DFAs           │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  State_A (from DFA1) and State_B (from DFA2)                 │     │
│  │                                                              │     │
│  │  Each state has:                                             │     │
│  │  ├─ input_constraint: Which inputs are possible?             │     │
│  │  └─ transition_fn: (input, output) → next_state_id           │     │
│  │                                                              │     │
│  │  Both are represented as ADDs for efficient combination       │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                │                                     │
│                                ▼                                     │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  Key Insight: Synchronized Product                           │     │
│  │                                                              │     │
│  │  The combined state (State_A, State_B) can ONLY             │     │
│  │  transition to (next_A, next_B) if BOTH conditions hold:    │     │
│  │                                                              │     │
│  │  1. State_A can transition to next_A under (input, output)  │     │
│  │  2. State_B can transition to next_B under SAME (input, output)│    │
│  │                                                              │     │
│  │  This is computed by: combined_ADD = ADD_A ∧ ADD_B           │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                │                                     │
│                                ▼                                     │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  Algorithm: Compute Combined Transitions                    │     │
│  │  ┌────────────────────────────────────────────────────────┐ │     │
│  │  │  // Both DFAs must agree on the input                  │ │     │
│  │  │  valid_inputs = input_A ∧ input_B                       │ │     │
│  │  │                                                        │ │     │
│  │  │  // For each valid (input, output) pair:               │ │     │
│  │  │  for each (in, out) in valid_inputs:                    │ │     │
│  │  │    next_A = transition_fn_A(in, out)                    │ │     │
│  │  │    next_B = transition_fn_B(in, out)                    │ │     │
│  │  │    add_successor(next_A, next_B)                         │     │
│  │  │                                                        │ │     │
│  │  │  // Result: set of reachable combined successors        │     │
│  │  └────────────────────────────────────────────────────────┘ │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                │                                     │
│                                ▼                                     │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  Example: Concrete Numbers                                 │     │
│  │  ┌────────────────────────────────────────────────────────┐ │     │
│  │  │  State_A (q1_3) has 2 successors:                       │ │     │
│  │  │    (in1, out1) → q1_5                                   │ │     │
│  │  │    (in2, out2) → q1_7                                   │ │     │
│  │  │                                                        │ │     │
│  │  │  State_B (q2_2) has 3 successors:                       │ │     │
│  │  │    (in1, out1) → q2_4                                   │ │     │
│  │  │    (in1, out3) → q2_8                                   │ │     │
│  │  │    (in2, out2) → q2_1                                   │ │     │
│  │  │                                                        │ │     │
│  │  │  Combined state (q1_3, q2_2) has 2 successors:           │     │
│  │  │    (in1, out1) → (q1_5, q2_4)  ← BOTH agree!            │ │     │
│  │  │    (in2, out2) → (q1_7, q2_1)  ← BOTH agree!            │ │     │
│  │  │                                                        │ │     │
│  │  │  Note: (in1, out3) from State_B is NOT in result       │     │
│  │  │  ┌────────────────────────────────────────────────────┐     │
│  │  │  │ Why? State_A with (in1, out3) leads to FALSE/Ewin  │     │
│  │  │  │ (a losing state in AWR - Agent Winning Region)     │     │
│  │  │  │                                                      │     │
│  │  │  │ In game theory: if one component loses,            │     │
│  │  │  │ the combined system loses. No need to compute      │     │
│  │  │  │ the other component's outcome - it's irrelevant!    │     │
│  │  │  │                                                      │     │
│  │  │  │ The ADD conjunction automatically filters this:     │     │
│  │  │  │ input_A ∧ input_B removes (in1, out3) because       │     │
│  │  │  │ State_A has no valid transition there             │     │
│  │  └────────────────────────────────────────────────────┘     │
│  │  └────────────────────────────────────────────────────────┘ │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                │                                     │
│                                ▼                                     │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  Encoding Combined State IDs                                │     │
│  │  ┌────────────────────────────────────────────────────────┐ │     │
│  │  │  To represent (q1_id, q2_id) as a single integer:      │ │     │
│  │  │                                                        │ │     │
│  │  │  combined_id = (q1_id << offset) | q2_id               │ │     │
│  │  │                                                        │ │     │
│  │  │  Where offset = number of bits needed for DFA2         │     │
│  │  │                                                        │ │     │
│  │  │  Example: offset = 10 (DFA2 has < 1024 states)          │     │
│  │  │    (q1_5, q2_4) = (5 << 10) | 4 = 5124               │     │
│  │  │    (q1_7, q2_1) = (7 << 10) | 1 = 7169               │     │
│  │  └────────────────────────────────────────────────────────┘ │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### Why This Works

The ADD conjunction (`input_A ∧ input_B`) automatically handles:
- **Intersection**: Only inputs valid for BOTH states are kept
- **Filtering**: Transitions that don't align are discarded
- **Efficiency**: ADD operations are optimized for this kind of symbolic computation


## Edge Constraint BDD Structure

```
┌─────────────────────────────────────────────────────────────────────┐
│              EDGE CONSTRAINT USING BDDs                             │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Variables:                                                         │
│  ├─ X = {x1, x2, ..., xn}  (Input/Environment variables)           │
│  └─ Y = {y1, y2, ..., ym}  (Output/System variables)               │
│                                                                      │
│  For each state q with successors {q1, q2, ..., qk}:                │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  EdgeCons:                                                  │     │
│  │  ├─ succIdSet = {hashId(q1), hashId(q2), ...}              │     │
│  │  ├─ afX: BDD over X (input assignments)                    │     │
│  │  └─ afY: BDD over X ∪ Y (transition constraints)            │     │
│  │                                                               │     │
│  │  Winning condition check:                                     │     │
│  │     ├─ Swin: ∀X, ∃Y such that all successors win            │     │
│  │     └─ Ewin: ∃X such that ∀Y, some successor loses          │     │
│  │                                                               │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
│  Example:                                                           │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  State q has 2 successors:                                  │     │
│  │  ├─ To q1: (x1 ∧ y1) ∨ (¬x1 ∧ y2)                          │     │
│  │  └─ To q2: (x1 ∧ ¬y1) ∨ (¬x1 ∧ ¬y2)                        │     │
│  │                                                               │     │
│  │  afY encodes all transitions as BDD:                         │     │
│  │  BDD = (x1 ∧ y1 ∧ q1) ∨ (¬x1 ∧ y2 ∧ q1) ∨                  │     │
│  │        (x1 ∧ ¬y1 ∧ q2) ∨ (¬x1 ∧ ¬y2 ∧ q2)                  │     │
│  │                                                               │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## Backward Search Propagation

```
┌─────────────────────────────────────────────────────────────────────┐
│              BACKWARD SEARCH IN SCC                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Given SCC with states {s1, s2, ..., sn}:                           │
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  Initialize:                                                │     │
│  │  ├─ cur_swin = {s | s marked as Swin}                      │     │
│  │  ├─ undecided = {s | s marked as Dfs_complete}              │     │
│  │  └─ predecessors: reverse edge map                          │     │
│  └────────────────────────────┬─────────────────────────────────┘     │
│                               ▼                                      │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  REPEAT until no change:                                   │     │
│  │  ┌────────────────────────────────────────────────────┐    │     │
│  │  │  new_swin = {}                                      │    │     │
│  │  │                                                      │    │     │
│  │  │  FOR each s in cur_swin:                            │    │     │
│  │  │    FOR each p in predecessors(s):                   │    │     │
│  │  │      IF p in undecided:                             │    │     │
│  │  │        IF p can force to cur_swin:                  │    │     │
│  │  │          mark p as Swin                              │    │     │
│  │  │          add p to new_swin                           │    │     │
│  │  │          remove p from undecided                     │    │     │
│  │  │        ELSE IF p cannot avoid losing:                │    │     │
│  │  │          mark p as Ewin                              │    │     │
│  │  │          remove p from undecided                     │    │     │
│  │  │                                                      │    │     │
│  │  │  cur_swin = new_swin                                 │    │     │
│  │  └────────────────────────────────────────────────────┘    │     │
│  │                                                             │     │
│  └────────────────────────────────────────────────────────────┘     │
│                               ▼                                      │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │  FINALLY:                                                   │     │
│  │  FOR each s in undecided:                                  │     │
│  │    mark s as Ewin                                          │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
│  Winning Condition:                                                 │
│  ├─ Swin: ∀X ∈ inputs, ∃Y ∈ outputs, succ(s, X, Y) ⊆ Swin        │
│  └─ Ewin: ∃X ∈ inputs, ∀Y ∈ outputs, ∃s' ∈ succ(s, X, Y), s' ∈ Ewin│
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## Simplified State Status Flow

```
                        ┌──────────────┐
                        │   Unvisited   │
                        └───────┬──────┘
                                │ visitNode()
                                ▼
                   ┌────────────────────────┐
                   │ Check Empty Acceptance  │
                   └────────────┬───────────┘
                                │
                    ┌───────────┴───────────┐
                    ▼                       ▼
             ┌─────────────┐         ┌─────────────────┐
             │  Empty Acc  │         │ Not Empty Acc   │
             │  → Swin ✓   │         │                 │
             └──────┬──────┘         └────────┬────────┘
                    │                          │
                    │                          ▼
                    │                 ┌─────────────────┐
                    │                 │ Build Edge Cons │
                    │                 │   (BDD based)   │
                    │                 └────────┬────────┘
                    │                          │
                    │                          ▼
                    │                 ┌─────────────────┐
                    │                 │ Dfs_incomplete  │
                    │                 └────────┬────────┘
                    │                          │
                    │                          ▼
                    │                 ┌─────────────────────────────┐
                    │                 │   WHILE more edges exist:   │
                    │                 │   ├─ getNextEdge()          │
                    │                 │   │  └→ Get next edge label │
                    │                 │   └─ make_trans()          │
                    │                 │      └→ Compute successor  │
                    │                 │                              │
                    │                 │   (Recursively explore)     │
                    │                 │         ↓                    │
                    │                 │   Push successor to DFS    │
                    │                 │   stack and visit it        │
                    │                 └──────────────┬──────────────┘
                    │                                │
                    │                 ┌──────────────┴──────────┐    │
                    │                 ▼                         ▼    │
                    │          All edges done         More edges   │    │
                    │                 │                         │    │
                    │                 ▼                         │    │
                    │          ┌─────────────────┐              │    │
                    │          │ Dfs_complete    │              │    │
                    │          └────────┬────────┘              │    │
                    │                   │                       │    │
                    └───────────────────┼───────────────────────┘    │
                                        │                            │
                                        ▼                            │
                              ┌────────────────────┐                  │
                              │   processScc()     │                  │
                              │   (SCC found)      │                  │
                              └─────────┬──────────┘                  │
                                        │                              │
                                        ▼                              │
                              ┌────────────────────┐                  │
                              │  backwardSearch()  │                  │
                              └─────────┬──────────┘                  │
                                        │                              │
                              ┌─────────┴────────┐                     │
                              ▼                  ▼                     │
                        ┌──────────┐       ┌──────────┐               │
                        │   Swin   │       │   Ewin   │               │
                        │(Winning) │       │ (Losing) │               │
                        └──────────┘       └──────────┘               │
                                                                  │
                            ┌───────────────────────────────────────┘
                            │
                            ▼
                    Return to predecessor in DFS stack
                    (continue its edge exploration loop)
```

### Edge Exploration Loop Detail

```
For state q with multiple outgoing edges:

┌─────────────────────────────────────────────────────────────────┐
│  State q in Dfs_incomplete                                       │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  LOOP: Get next edge                                      │  │
│  │  ├─ edge = getNextEdge(q)                                 │  │
│  │  │                                                        │  │
│  │  ├─ IF edge exists: (More edges)                          │  │
│  │  │  ├─ next = make_trans(q, edge)                         │  │
│  │  │  │     (Compute successor state)                       │  │
│  │  │  │                                                     │  │
│  │  │  ├─ IF next not visited:                               │  │
│  │  │  │  ├─ Push next to DFS stack                          │  │
│  │  │  │  └─ Visit next (recursive/iterative)                │  │
│  │  │  │                                                     │  │
│  │  │  └─ Continue LOOP (check for more edges)              │  │
│  │  │                                                        │  │
│  │  └─ ELSE: (All edges done)                                │  │
│  │     └─ Mark q as Dfs_complete                             │  │
│  │                                                        │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                           │       │
│  After Dfs_complete, wait for:                              │       │
│  ├─ SCC detection (when dfn == low)                         │       │
│  └─ backwardSearch() to determine final status              │       │
└───────────────────────────────────────────────────────────────────┘
                            │
                            ▼
              Pop from DFS stack, return to predecessor
```

### DFS Stack Behavior

```
DFS Stack shows exploration order (top = current):

Example: q0 → q1 → q2 → q3

┌─────────┐
│   q3    │ ← Currently exploring q3's edges
├─────────┤
│   q2    │ ← Waiting for q3 to complete
├─────────┤
│   q1    │ ← Waiting for q2 to complete
├─────────┤
│   q0    │ ← Waiting for q1 to complete
└─────────┘

When q3 becomes Dfs_complete:
┌─────────┐
│   q2    │ ← Resumes exploring its remaining edges
├─────────┤
│   q1    │
├─────────┤
│   q0    │
└─────────┘
```

## Component Interaction Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                    CORE COMPONENT INTERACTIONS                      │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│   main.cpp                                                           │
│      │                                                               │
│      ├─> read_ltlf_file()                                           │
│      ├─> read_part_file()                                           │
│      │                                                               │
│      └─> compositional_synthesis1()  ────────────────────┐          │
│                                                           │          │
│   dfs_product.cpp                                          │          │
│      │                                                      │          │
│      ├─> getAndSubAfs()                                   │          │
│      │                                                      │          │
│      ├─> [LOOP]                                            │          │
│      │    │                                                 │          │
│      │    ├─> WholeDFA_TarjanStrategy::doSynthesis()       │          │
│      │    │    │                                            │          │
│      │    │    ├─> Tarjan::dfsSearch()                     │          │
│      │    │    │    │                                       │          │
│      │    │    │    ├─> visitNode()                         │          │
│      │    │    │    │    └─> EdgeConsBuilder::build()      │          │
│      │    │    │    │         └─> BDD operations           │          │
│      │    │    │    │                                       │          │
│      │    │    │    ├─> make_trans()                       │          │
│      │    │    │    │    └─> rmnext() formula              │          │
│      │    │    │    │                                       │          │
│      │    │    │    └─> processScc()                       │          │
│      │    │    │         └─> backwardSearch()              │          │
│      │    │    │                                            │          │
│      │    │    └─> Return: Swin/Ewin                       │          │
│      │    │                                                 │          │
│      │    └─> WholeDFAComb_TarjanStrategy::doSynthesis()    │          │
│      │         │                                            │          │
│      │         ├─> Import DFA 1 & DFA 2                     │          │
│      │         │                                            │          │
│      │         ├─> Tarjan::dfsSearch(combined DFA)          │          │
│      │         │    │                                       │          │
│      │         │    ├─> visitNode(combined state)          │          │
│      │         │    ├─> make_trans(combined)               │          │
│      │         │    └─> processScc(combined)               │          │
│      │         │                                            │          │
│      │         └─> Return: Combined Swin/Ewin              │          │
│      │                                                      │          │
│      └──────────────────────────────────────────────────────┘          │
│                                                           │           │
└───────────────────────────────────────────────────────────┼───────────┘
                                                            │
                                                            ▼
                                                      Result:
                                                      Realizable/
                                                     Unrealizable
```
