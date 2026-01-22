# [174] feat: add game graph visualization (DOT + JSON export)

**Commit**: `fb2a2f2` ([`fb2a2f2310f8aa09f8fa0de86a4db6a5326e11bf`](https://github.com/licoded/CosyZeroRewrite/commit/fb2a2f2310f8aa09f8fa0de86a4db6a5326e11bf))
**Date**: 2026-01-04 10:47:27 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add game graph export functionality for debugging synthesis issues.

Features:
- to_dot(): Export to GraphViz DOT format
  - System states: circles (blue border)
  - Environment states: boxes (orange border)
  - Swin: light green fill | Ewin: light red fill
  - Sys moves: blue solid lines | Env moves: red dashed lines
- to_json(): Export metadata with full formula information
- write_dot(): Write both .dot and .json files

Usage:
  COSY_DEBUG_GAME_GRAPH=1 ./build/output/Cosy2 "F(p)"
Output: results/game_graph/YYYY-MM-DD/HH-Period/game_graph_*.dot

Implementation details:
- Created separate game_graph_export.cpp for better code organization
- Methods declared in on_the_fly_solver.hpp, implemented in game_graph_export.cpp
- State IDs: S0, S1, ... for System; E0, E1, ... for Environment

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
添加游戏图可视化功能，用于调试 synthesis 问题。支持 DOT (GraphViz) 和 JSON 两种输出格式。

### 🔍 Technical Details
- **DOT 格式**: 用于图形化展示，区分 sys/env 状态和转移
  - System 状态: 圆形 (蓝色边框)
  - Environment 状态: 方形 (橙色边框)
  - Sys move: 蓝色实线, Env move: 红色虚线
  - Swin/Ewin 用不同填充颜色区分
- **JSON 格式**: 包含完整公式信息 (phi, xnf_phi, prop_atoms)
- **代码组织**: 独立文件 `game_graph_export.cpp`，保持主文件简洁
- **触发方式**: 环境变量 `COSY_DEBUG_GAME_GRAPH=1`

### 📊 Impact Analysis
- **调试能力提升**: 可视化游戏图帮助快速定位 synthesis 错误
- **性能影响**: 仅在启用环境变量时生成，不影响正常运行
- **输出位置**: `results/game_graph/YYYY-MM-DD/HH-Period/`

## Changes

### Added
- `src/synthesis/game_graph_export.cpp`


### Modified
- `cmake/Dependencies.cmake`
- `include/synthesis/on_the_fly_solver.hpp`
- `src/cosy2.cpp`
- `src/synthesis/on_the_fly_solver.cpp`


## Stats

- **5** files changed
- **462** insertions(+)
- **2** deletions(-)
