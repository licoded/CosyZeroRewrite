# [75] feat: add strategy extraction for LTLf synthesis

**Commit**: `776bf70` ([`776bf700bfe94b7714a0de5190b4dc21b4db4541`](https://github.com/licoded/CosyZeroRewrite/commit/776bf700bfe94b7714a0de5190b4dc21b4db4541))
**Date**: 2026-01-02 13:06:23 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implements strategy extraction from classified game graphs, allowing
users to visualize and verify winning strategies for realizable formulas.

Features:
- extract_strategy() method builds strategy graph via BFS from initial state
- find_winning_output() identifies system outputs that lead to winning states
- to_json() exports strategy as JSON for programmatic access
- to_dot() exports strategy as Graphviz DOT for visualization
- StrategyVerifier checks extracted strategy correctness

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
策略提取功能：实现从分类后的游戏图中提取获胜策略，支持 JSON 和 Graphviz DOT 导出。

### 🔍 Technical Details

**核心功能**：
- `extract_strategy()`: 通过 BFS 从初始状态构建策略图
- `find_winning_output()`: 识别导致获胜状态的系统输出
- `to_json()`: 导出策略为 JSON 格式
- `to_dot()`: 导出策略为 Graphviz DOT 用于可视化
- `StrategyVerifier`: 验证提取的策略正确性

**应用场景**：
- 可视化验证 winning strategy
- 策略调试和验证
- 生成可执行策略

## Changes

### Added
- `include/synthesis/strategy.hpp`
- `src/synthesis/strategy.cpp`
- `tests/strategy_extraction_test.cpp`


### Modified
- `cmake/Dependencies.cmake`
- `cmake/Tests.cmake`


## Stats

- **5** files changed
- **690** insertions(+)
- **1** deletions(-)
