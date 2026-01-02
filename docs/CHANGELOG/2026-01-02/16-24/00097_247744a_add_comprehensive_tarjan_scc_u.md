# [97] test(synthesis): add comprehensive Tarjan SCC unit tests

**Commit**: `247744a` ([`247744ac20e0406c93f9deb7d44694faf5eb23c3`](https://github.com/licoded/CosyZeroRewrite/commit/247744ac20e0406c93f9deb7d44694faf5eb23c3))
**Date**: 2026-01-02 20:05:47 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add tarjan_scc_tests.cpp with 13 test cases covering:
  * Single node, linear chain, simple cycle
  * Two connected cycles, self-loop
  * Complex DAG, cross pattern
  * SCC with incoming/outgoing edges
  * Topological order property

- Add find_sccs_for_testing() as separate implementation
  that doesn't trigger expand_state(), keeping production
  code unchanged (option 2 approach)

- Fix add_test_transition() to append rather than replace
  successor lists

- Add comprehensive algorithm documentation:
  * docs/ARCHITECTURE/on_the_fly_algorithm.md - Complete
    rewrite with correct algorithm understanding
  * docs/ARCHITECTURE/tarjan_scc_algorithm.md - Tarjan
    algorithm research and implementation analysis

- Update docs/BUGS/fixed.md with Bug #003 details

All tests pass: 13/13 test cases, 48 assertions

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
添加了全面的 Tarjan SCC 算法单元测试，包含 13 个测试用例覆盖各种图结构。同时添加了算法文档和正确的 on-the-fly 算法说明文档。

### 🔍 Technical Details
- 创建 `find_sccs_for_testing()` 作为独立测试接口，避免触发 `expand_state()`
- 修复 `add_test_transition()` 从替换改为追加后继列表
- 添加 `docs/ARCHITECTURE/tarjan_scc_algorithm.md` 算法研究文档
- 重写 `docs/ARCHITECTURE/on_the_fly_algorithm.md` 修正算法理解错误

### 📊 Impact Analysis
- 确保 Tarjan SCC 实现正确性
- 为后续优化（如增量 SCC 算法）提供测试基础
- 记录了之前的状态传播规则错误和 SCC 分类错误（Bug #003）

## Changes

### Added
- `docs/ARCHITECTURE/on_the_fly_algorithm.md`
- `docs/ARCHITECTURE/tarjan_scc_algorithm.md`
- `tests/tarjan_scc_tests.cpp`


### Modified
- `cmake/Tests.cmake`
- `docs/BUGS/fixed.md`
- `include/synthesis/on_the_fly_solver.hpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **7** files changed
- **1761** insertions(+)
- **56** deletions(-)
