# [177] feat: add interactive game graph visualization with webgraphviz

**Commit**: `3436b38` ([`3436b38166cf3200bc8347e65aa3c8beb20a7d08`](https://github.com/licoded/CosyZeroRewrite/commit/3436b38166cf3200bc8347e65aa3c8beb20a7d08))
**Date**: 2026-01-04 12:04:02 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Based on zackees/webgraphviz, the game graph can now be exported as
a self-contained HTML file with:
- Interactive SVG graph rendering using embedded viz.js
- Hover tooltips showing state details (phi, xnf_phi, prop_atoms, classification)
- Visual distinction between System (circle, blue) and Environment (box, orange) states
- Color coding for Swin (green) and Ewin (red) states

Technical changes:
- Added resources/viz.js from webgraphviz project
- Implemented to_html() method with DOT escaping for JS template literals
- Added PROJECT_SOURCE_DIR compile definition for resource path
- Generated HTML is self-contained with embedded viz.js (~1.4MB)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
<!-- TODO: Add a brief summary of the change in Chinese or English -->

### 🔍 Technical Details
<!-- Optional: Add technical details, root cause, or implementation notes -->

### 📊 Impact Analysis
<!-- Optional: Add impact scope, affected components, or performance notes -->

## Changes

### Added
- `resources/CTestTestfile.cmake`
- `resources/output/benchmark_runner`
- `resources/output/Cosy2`
- `resources/tests/automata/dfa_test`
- `resources/tests/automata/progression_test`
- `resources/tests/automata/tarjan_test`
- `resources/tests/bench/benchmark_test`
- `resources/tests/debug/eventually_contradiction`
- `resources/tests/debug/failing_tests`
- `resources/tests/formula/formula_test`
- `resources/tests/formula/transformation_test`
- `resources/tests/fuzz/nnf_fuzz`
- `resources/tests/fuzz/random_fuzz`
- `resources/tests/fuzz/xnf_fuzz`
- `resources/tests/integration/io_separation_test`
- `resources/tests/integration/prop_atoms_test`
- `resources/tests/integration/strategy_test`
- `resources/tests/parser/parser_test`
- `resources/tests/synthesis/on_the_fly_test`
- `resources/tests/synthesis/synthesis_test`
- `resources/tests/transformation/nnf_test`
- `resources/tests/transformation/xnf_test`
- `resources/viz.js`


### Modified
- `cmake/CompilerOptions.cmake`
- `src/synthesis/game_graph_export.cpp`


## Stats

- **25** files changed
- **125** insertions(+)
- **29** deletions(-)
