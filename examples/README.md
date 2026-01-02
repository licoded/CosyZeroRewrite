# CosyZero Synthesis Examples

This directory contains example LTLf formulas for testing the synthesis tool.

## File Format

Each example file should have:
- `.ltlf` extension for the formula
- Optional `.part` extension for variable partition (input/output)
- `.desc` extension for human-readable description

## Running Synthesis

```bash
# Using Cosy2 binary
Cosy2 -f example.ltlf [-p example.part]

# Or with inline formula
Cosy2 "F (req -> F ack)"
```

## Examples

| File | Description |
|------|-------------|
| `response.ltlf` | Response pattern: G(request -> F acknowledge) |
| `sequence.ltlf` | Sequential execution: p1 & X p2 |
| `arbitration.ltlf` | Arbitration: (req1 -> F ack1) & (req2 -> F ack2) |

## Formula Syntax

- `true`, `false`: Boolean constants
- `p1`, `p2`, ...: Propositional variables
- `!p`: Negation
- `p & q`: Conjunction
- `p \| q`: Disjunction (use `\|` in shell)
- `X p`: Next (p must hold at next step)
- `F p`: Eventually (p must hold at some future step)
- `G p`: Globally/Always (p must hold at all steps)
- `p U q`: Until (p holds until q becomes true)
- `p R q`: Release (q holds until p becomes true, then p must hold)

## Semantics

- **LTLf**: Linear Temporal Logic over Finite Traces
- **Synthesis**: Find a strategy for the system to satisfy the formula
  - **Realizable**: A winning strategy exists
  - **Unrealizable**: No strategy can satisfy the formula
