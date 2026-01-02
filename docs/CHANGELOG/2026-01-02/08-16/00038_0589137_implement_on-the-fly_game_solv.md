# [38] feat: implement on-the-fly game solver

**Commit**: `0589137` ([`058913734dfa9bf1bf46b1dc38c7b623364dcb4f`](https://github.com/anthropics/cosy-zero/commit/058913734dfa9bf1bf46b1dc38c7b623364dcb4f))
**Date**: 2026-01-02 09:13:57 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implement the on-the-fly game solving algorithm (Algorithm 2 from
arXiv:2408.07324) with SCC decomposition and state classification.

- GameState: Game state with DFA state, player, and output assignment
- OnTheFlyGameSolver: Main solver with lazy state expansion
- Tarjan's algorithm: SCC decomposition on-the-fly
- State classification: Swin/Ewin based on accepting DFA states
- Backward propagation: Classify predecessors from SCCs

Key features:
- Early termination when initial state classified
- Lazy state expansion (only when needed)
- Proper SCC classification rules
- Backward propagation for both system and environment states

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `include/synthesis/game_solver.hpp`
- `include/synthesis/on_the_fly_solver.hpp`
- `include/synthesis/synthesis.hpp`
- `src/synthesis/game_solver.cpp`
- `src/synthesis/on_the_fly_solver.cpp`
- `src/synthesis/synthesis.cpp`


## Stats

- **6** files changed
- **1455** insertions(+)
- **0** deletions(-)
