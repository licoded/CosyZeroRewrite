# [176] feat: add interactive HTML visualization for game graph

**Commit**: `91fd321` ([`91fd32160ce44d8fa1c6e00f66beaf5bf8579e22`](https://github.com/licoded/CosyZeroRewrite/commit/91fd32160ce44d8fa1c6e00f66beaf5bf8579e22))
**Date**: 2026-01-04 11:03:25 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add self-contained HTML output with interactive tooltips.

Features:
- Single HTML file (no external dependencies except viz.js CDN)
- Mouse hover shows full state information (phi, xnf_phi, prop_atoms)
- Clean UI with statistics, legend, and styled tooltips
- Works in any modern browser - just double-click to open

Technical details:
- to_html(): Generates complete HTML with embedded DOT and state data
- escape_html(): Helper function for HTML escaping
- Uses viz.js from CDN for graph rendering
- Interactive tooltips built with vanilla JavaScript

Usage:
  COSY_DEBUG_GAME_GRAPH=1 ./build/output/Cosy2 "F(p)"
Output includes: .dot, .json, .html

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
添加交互式 HTML 可视化功能。鼠标悬浮即可查看完整状态信息（phi、xnf_phi、prop_atoms），无需对照 JSON 文件。

### 🔍 Technical Details
- **单文件 HTML**: 内嵌 DOT 内容和状态数据，双击即用
- **viz.js 渲染**: 从 CDN 加载，无需本地安装 GraphViz
- **交互式 Tooltip**: 鼠标悬浮显示完整公式信息
- **响应式布局**: 统计信息、图例、样式化提示框
- **HTML 转义**: escape_html() 函数防止 XSS

### 📊 Impact Analysis
- **调试效率大幅提升**: 不再需要来回切换文件查看公式
- **用户体验**: 现代化界面，直观的视觉反馈
- **跨平台**: 任何浏览器都可以打开

## Changes

### Modified
- `include/synthesis/on_the_fly_solver.hpp`
- `src/cosy2.cpp`
- `src/synthesis/game_graph_export.cpp`


## Stats

- **3** files changed
- **365** insertions(+)
- **5** deletions(-)
