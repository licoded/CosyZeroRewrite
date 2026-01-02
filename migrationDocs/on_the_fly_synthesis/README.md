# On-the-Fly LTLf Synthesis

**Based on**: arXiv:2408.07324 - "On-the-fly Synthesis for LTL over Finite Traces" (2024)

**Authors**: S. Xiao, M. Xiao, et al.

## Overview

This document describes the on-the-fly synthesis algorithm for LTLf (Linear Temporal Logic over Finite Traces). The key innovation is constructing the DFA and solving the synthesis game **on-demand**, avoiding full automaton construction.

## Key Ideas

### 1. Top-Down DFA Construction

Traditional approaches (AALTA, Lydia) build the complete DFA first, then solve the game. On-the-fly synthesis:

- Constructs DFA states **only when needed** during game solving
- Uses tableau-based construction for LTLf → DFA conversion
- Early termination when realizability is determined

### 2. Game-Based Synthesis

LTLf synthesis is formulated as a **two-player game**:
- **System player (S)**: Controls output variables
- **Environment player (E)**: Controls input variables

### 3. SCC-Based Winning Region Detection

Uses **Tarjan's algorithm** for Strongly Connected Components (SCC) decomposition:
- States in accepting SCCs → Winning (Swin)
- Backward propagation to classify remaining states
- Early termination if initial state is classified

---

## Algorithm Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    On-the-Fly LTLf Synthesis                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  LTLf Formula φ                                                   │
│       │                                                           │
│       ▼                                                           │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  Step 1: NNF Conversion                                     │ │
│  │  φ → nnf(φ)  [push negation inward]                         │ │
│  └─────────────────────────────────────────────────────────────┘ │
│       │                                                           │
│       ▼                                                           │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  Step 2: On-the-Fly Tableau Construction                    │ │
│  │  - Start from initial state q₀ = {φ}                         │ │
│  │  - Expand states lazily during game solving                 │ │
│  │  - Each state: set of subformulas                           │ │
│  └─────────────────────────────────────────────────────────────┘ │
│       │                                                           │
│       ▼                                                           │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  Step 3: Game Graph Construction (On-Demand)               │ │
│  │  - System moves: choose output assignment                   │ │
│  │  - Environment moves: choose input assignment                │ │
│  │  - δ(q, a, b) = expand(state, a ∪ b)                        │ │
│  └─────────────────────────────────────────────────────────────┘ │
│       │                                                           │
│       ▼                                                           │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  Step 4: SCC Decomposition + Classification                │ │
│  │  - Find SCCs using Tarjan's algorithm                       │ │
│  │  - Classify: Swin (winning) / Ewin (losing) / Unknown       │ │
│  │  - Backward propagation from classified states              │ │
│  └─────────────────────────────────────────────────────────────┘ │
│       │                                                           │
│       ▼                                                           │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  Step 5: Strategy Extraction (if realizable)               │ │
│  │  - Extract winning moves from Swin states                   │ │
│  │  - Build transducer strategy                                │ │
│  └─────────────────────────────────────────────────────────────┘ │
│       │                                                           │
│       ▼                                                           │
│  Realizable? / Strategy                                          │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## Tableau Construction (Section 3)

### State Representation

Each DFA state is a **set of subformulas**:

```
q = { ψ₁, ψ₂, ..., ψₙ }
```

where each ψ is a subformula of the original LTLf formula φ.

### Initial State

```
q₀ = { nnf(φ) }
```

### Expansion Rules (Tableau 1)

For state q = Γ:

| If condition | Then add to Γ' (next state) |
|-------------|---------------------------|
| `true ∈ Γ` | nothing |
| `false ∈ Γ` | nothing |
| `a ∈ Γ` (literal) | nothing |
| `ψ₁ ∧ ψ₂ ∈ Γ` | `ψ₁, ψ₂ ∈ Γ` |
| `ψ₁ ∨ ψ₂ ∈ Γ` | `ψᵢ ∈ Γ` (choose one) |
| `○ψ ∈ Γ` (Next) | nothing |
| `ψ₁ 𝕌 ψ₂ ∈ Γ` (Until) | `ψ₂ ∈ Γ` OR (`ψ₁ ∈ Γ` AND `ψ₁ 𝕌 ψ₂ ∈ Γ`) |
| `ψ₁ 𝖱 ψ₂ ∈ Γ` (Release) | `ψ₁, ψ₂ ∈ Γ` OR (`ψ₂ ∈ Γ` AND `ψ₁ 𝖱 ψ₂ ∈ Γ`) |
| `¬ψ ∈ Γ` | nothing (NNF ensures no negated formulas) |

### Next-State Expansion (Tableau 2)

For current state Γ, the next state Γ' is:

```
Γ' = old(Γ) ∪ next(Γ)

where:
- old(Γ) = { ψ ∈ Γ : ψ is not of the form ○ψ or ψ₁ 𝕌 ψ₂ or ψ₁ 𝖱 ψ₂ }
- next(Γ) = { ψ' | ○ψ ∈ Γ and ψ' ∈ ψ } ∪
             { ψ₂ | ψ₁ 𝖱 ψ₂ ∈ Γ }
```

### State Satisfaction

A state Γ is **satisfied** (accepting) if:
- No `false` in Γ
- No `ψ₁ 𝕌 ψ₂` in Γ where ψ₁ is not in Γ

### Local Consistency

Before expanding a state, check local consistency:

```python
def is_locally_consistent(Γ):
    if false in Γ:
        return False
    for (ψ1 ∧ ψ2) in Γ:
        if ψ1 not in Γ or ψ2 not in Γ:
            return False
    for (ψ1 ∨ ψ2) in Γ:
        if ψ1 not in Γ and ψ2 not in Γ:
            return False
    for (ψ1 U ψ2) in Γ:
        if ψ2 not in Γ and (ψ1 not in Γ or (ψ1 U ψ2) not in Γ):
            return False
    return True
```

---

## Game Solving (Section 4)

### Game Graph Structure

```
Game Graph G = (V, V_S, V_E, δ, v₀)

where:
- V: all game states (DFA states × player to move)
- V_S: system's turn states
- V_E: environment's turn states
- δ: transition function
- v₀: initial state
```

### Winning Condition

A play is winning for the system if it ends in an **accepting DFA state**.

### SCC-Based Classification

```python
def classify_scc(SCC):
    # Check if SCC contains an accepting DFA state
    has_accepting = any(is_accepting(q) for q in SCC)

    if has_accepting:
        # All states in this SCC are winning for system
        for state in SCC:
            state.type = Swin
    else:
        # All states in this SCC are losing for system
        for state in SCC:
            state.type = Ewin
```

### Backward Propagation

After SCC classification, propagate backwards:

```python
def backward_propagation():
    changed = True
    while changed:
        changed = False
        for state in unclassified_states:
            if is_system_turn(state):
                # System wins if ANY successor is winning
                if any(s.type == Swin for s in successors(state)):
                    state.type = Swin
                    changed = True
                elif all(s.type == Ewin for s in successors(state)):
                    state.type = Ewin
                    changed = True
            else:
                # Environment wins if ALL successors are winning for env
                if all(s.type == Ewin for s in successors(state)):
                    state.type = Ewin
                    changed = True
                elif any(s.type == Swin for s in successors(state)):
                    state.type = Swin
                    changed = True
```

---

## On-the-Fly Optimization

### Early Termination

```python
def on_the_fly_synthesis(φ):
    q0 = initial_state({nnf(φ)})
    worklist = [(q0, SYSTEM)]  # (state, player_to_move)
    visited = set()

    while worklist:
        state, player = worklist.pop()

        if state in visited:
            continue
        visited.add(state)

        # Check if we can classify this state
        classification = try_classify(state)

        if state == q0 and classification in [Swin, Ewin]:
            # Initial state classified - we're done!
            return classification == Swin

        # Expand successors on-demand
        for successor in get_successors(state, player):
            worklist.append((successor, next_player(player)))

    return False
```

### Lazy State Expansion

- Only expand DFA states when reached during game solving
- Prune branches that cannot affect realizability
- Cache expanded states to avoid recomputation

---

## Implementation Tasks

See [IMPLEMENTATION_PLAN.md](./IMPLEMENTATION_PLAN.md) for detailed implementation steps.

---

## References

- **Paper**: [arXiv:2408.07324](https://arxiv.org/abs/2408.07324) - "On-the-fly Synthesis for LTL over Finite Traces"
- **Related**: Forward LTLf Synthesis (arXiv:2302.13825) - DPLL-based approach
- **Related**: AALTA Tool - LTLf synthesis with symbolic BDD representation
- **Related**: Lydia - LTLf to DFA conversion

---

## Sources

- [arXiv:2408.07324 - On-the-fly Synthesis for LTL over Finite Traces](https://arxiv.org/abs/2408.07324)
- [arXiv:2302.13825 - Forward LTLf Synthesis: DPLL At Work](https://arxiv.org/abs/2302.13825)
- [arXiv:2508.04116 - A Compositional Framework for On-the-Fly LTLf Synthesis](https://arxiv.org/abs/2508.04116)
