# [249] refactor: clean up directory structure

**Commit**: `35ccc44` ([`35ccc44dbdb1edc62bad52358f2da14dc9ccbced`](https://github.com/licoded/CosyZeroRewrite/commit/35ccc44dbdb1edc62bad52358f2da14dc9ccbced))
**Date**: 2026-01-05 00:59:38 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Major directory cleanup and reorganization:

1. Move migration docs to docs/ARCHITECTURE/migration_docs/
   - Consolidate design documentation in a logical location
   - Keep historical migration docs with architecture docs

2. Remove obsolete directories:
   - benchmark_visualization/ - old HTML report (replaced by visualization/)
   - docs/TRACE_VISUALIZATION/ - old visualization docs
   - docs/build/ - empty directory
   - resources/ - old build directory (accidentally tracked)

3. Clean up local files:
   - tmp/ - old test binaries
   - logs/ - old log files (keep .gitkeep)
   - build_fuzz/ - old fuzz build directory
   - benchmark_results_latest.csv - move to results/

4. Update .gitignore:
   - Add resources/ to ignore list
   - Add more specific log file patterns
   - Add *.csv pattern for root directory

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
- `docs/ARCHITECTURE/migration_docs/formula_module/ADVANCED_OPTIMIZATION_TECHNIQUES.md`
- `docs/ARCHITECTURE/migration_docs/formula_module/ATOMIC_VARIABLE_HANDLING.md`
- `docs/ARCHITECTURE/migration_docs/formula_module/BDD_CONSIDERATIONS.md`
- `docs/ARCHITECTURE/migration_docs/formula_module/CORE_FUNCTIONALITY.md`
- `docs/ARCHITECTURE/migration_docs/formula_module/FORMULA_OPERATIONS.md`
- `docs/ARCHITECTURE/migration_docs/formula_module/LTLf_BASIC.md`
- `docs/ARCHITECTURE/migration_docs/formula_module/MEMORY_MANAGEMENT_DECISION.md`
- `docs/ARCHITECTURE/migration_docs/formula_module/PROPOSAL_B_ARCHITECTURE.md`
- `docs/ARCHITECTURE/migration_docs/formula_module/REDESIGN_PROPOSALS.md`
- `docs/ARCHITECTURE/migration_docs/formula_module/SIMPLIFY_IMPLEMENTATION.md`
- `docs/ARCHITECTURE/migration_docs/formula_redesign/FLOWCHARTS.md`
- `docs/ARCHITECTURE/migration_docs/formula_redesign/FORMULA_REWRITE_DESIGN.md`
- `docs/ARCHITECTURE/migration_docs/formula_redesign/HASH_CONSING_ANALYSIS.md`
- `docs/ARCHITECTURE/migration_docs/formula_redesign/IMPLEMENTATION_TASKS.md`
- `docs/ARCHITECTURE/migration_docs/formula_redesign/NNF_TRANSFORMATION.md`
- `docs/ARCHITECTURE/migration_docs/formula_redesign/README.md`
- `docs/ARCHITECTURE/migration_docs/formula_redesign/SIMPLIFY_ANALYSIS.md`
- `docs/ARCHITECTURE/migration_docs/formula_redesign/TODOs.md`
- `docs/ARCHITECTURE/migration_docs/formula_redesign/XNF_TRANSFORMATION.md`
- `docs/ARCHITECTURE/migration_docs/on_the_fly_synthesis/IMPLEMENTATION_PLAN.md`
- `docs/ARCHITECTURE/migration_docs/on_the_fly_synthesis/README.md`
- `docs/ARCHITECTURE/migration_docs/on_the_fly_synthesis/TABLEAU_DFA.md`
- `docs/ARCHITECTURE/migration_docs/README.md`
- `docs/ARCHITECTURE/migration_docs/reimplementation/NEW_REPOSITORY_SPEC.md`
- `docs/ARCHITECTURE/migration_docs/system_architecture/COMB_FULL_1_COMP_IDX_1_ANALYSIS.md`
- `docs/ARCHITECTURE/migration_docs/system_architecture/EDGE_CONSTRAINT_SPEC.md`
- `docs/ARCHITECTURE/migration_docs/system_architecture/FLOWCHART.md`
- `docs/ARCHITECTURE/migration_docs/system_architecture/SUMMARY.md`
- `docs/working_issues/2026-01-05_AM_DirectoryCleanup/ANALYSIS.md`


### Modified
- `.gitignore`


### Deleted
- `docs/TRACE_VISUALIZATION/implementation_plan.md`
- `docs/TRACE_VISUALIZATION/json_schema.md`
- `docs/TRACE_VISUALIZATION/README.md`
- `migrationDocs/formula_module/ADVANCED_OPTIMIZATION_TECHNIQUES.md`
- `migrationDocs/formula_module/ATOMIC_VARIABLE_HANDLING.md`
- `migrationDocs/formula_module/BDD_CONSIDERATIONS.md`
- `migrationDocs/formula_module/CORE_FUNCTIONALITY.md`
- `migrationDocs/formula_module/FORMULA_OPERATIONS.md`
- `migrationDocs/formula_module/LTLf_BASIC.md`
- `migrationDocs/formula_module/MEMORY_MANAGEMENT_DECISION.md`
- `migrationDocs/formula_module/PROPOSAL_B_ARCHITECTURE.md`
- `migrationDocs/formula_module/REDESIGN_PROPOSALS.md`
- `migrationDocs/formula_module/SIMPLIFY_IMPLEMENTATION.md`
- `migrationDocs/formula_redesign/FLOWCHARTS.md`
- `migrationDocs/formula_redesign/FORMULA_REWRITE_DESIGN.md`
- `migrationDocs/formula_redesign/HASH_CONSING_ANALYSIS.md`
- `migrationDocs/formula_redesign/IMPLEMENTATION_TASKS.md`
- `migrationDocs/formula_redesign/NNF_TRANSFORMATION.md`
- `migrationDocs/formula_redesign/README.md`
- `migrationDocs/formula_redesign/SIMPLIFY_ANALYSIS.md`
- `migrationDocs/formula_redesign/TODOs.md`
- `migrationDocs/formula_redesign/XNF_TRANSFORMATION.md`
- `migrationDocs/on_the_fly_synthesis/IMPLEMENTATION_PLAN.md`
- `migrationDocs/on_the_fly_synthesis/README.md`
- `migrationDocs/on_the_fly_synthesis/TABLEAU_DFA.md`
- `migrationDocs/README.md`
- `migrationDocs/reimplementation/NEW_REPOSITORY_SPEC.md`
- `migrationDocs/system_architecture/COMB_FULL_1_COMP_IDX_1_ANALYSIS.md`
- `migrationDocs/system_architecture/EDGE_CONSTRAINT_SPEC.md`
- `migrationDocs/system_architecture/FLOWCHART.md`
- `migrationDocs/system_architecture/SUMMARY.md`
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


## Stats

- **84** files changed
- **16004** insertions(+)
- **17015** deletions(-)
