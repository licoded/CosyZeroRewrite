# [107] refactor: implement two-phase SCC strategy with consistency checking

**Commit**: `2b6f693` ([`2b6f6937634c93eaf38371a1f192cc10c8c78092`](https://github.com/licoded/CosyZeroRewrite/commit/2b6f6937634c93eaf38371a1f192cc10c8c78092))
**Date**: 2026-01-02 20:32:44 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

修改 on_the_fly_game_solver 为两阶段策略：

Phase 1: 展开所有可达状态（不提前做 SCC/classify/propagate）
Phase 2: 在完整图上做 SCC 分解和分类
Phase 3: Terminal 状态分类
Phase 4: 传播到不动点

新增功能：
- check_propagation_consistency() - 检查传播逻辑一致性
- COSY_DEBUG_PROPAGATION 环境变量启用一致性检查

注意：当前有 2 个测试失败 (not_literal, response_formula)
需要进一步调试

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

### Modified
- `include/synthesis/on_the_fly_solver.hpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **2** files changed
- **208** insertions(+)
- **93** deletions(-)
