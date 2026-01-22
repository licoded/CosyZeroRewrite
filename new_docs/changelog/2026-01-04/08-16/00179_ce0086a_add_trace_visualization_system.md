# [179] docs: add trace visualization system design

**Commit**: `ce0086a` ([`ce0086ace6cec1526df18930dfede16d52b6e5e8`](https://github.com/licoded/CosyZeroRewrite/commit/ce0086ace6cec1526df18930dfede16d52b6e5e8))
**Date**: 2026-01-04 12:26:23 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

添加了 Web-based Trace Visualization System 的完整设计文档：

## 新增文件

### docs/TRACE_VISUALIZATION/README.md
- 系统架构概述 (C++ JSON Exporter + Web Visualizer)
- 执行步骤定义 (expand, SCC, fixed-point)
- 多层高亮方案 (节点类型 + 胜利区域 + 动态变化)
- 输出位置规范
- 设计决策记录 (Q1-Q5)

### docs/TRACE_VISUALIZATION/json_schema.md
- 完整的 JSON Schema 定义
- Stage/Sub-Step/GraphData/Highlights/StateInfo 结构
- TypeScript 类型定义
- 完整示例

### docs/TRACE_VISUALIZATION/implementation_plan.md
- 4 个开发阶段的详细计划
- Phase 1: C++ Trace Exporter
- Phase 2: 前端基础 (Vite + Vue 3 + TypeScript)
- Phase 3: 交互功能 (StepTree, GraphCanvas, Stepper)
- Phase 4: 优化与集成

## 关键设计决策

- Q1: 存储完整 DOT (方案 A) - 简单直接
- Q2: 多层高亮 + SVG DOM 操作
- Q3: 增量写入 JSON 单文件
- Q4: Vite + Vue 3 + TypeScript
- Q5: 左侧树形导航 + 底部步进器

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary

为 LTLf Synthesis 执行过程设计了完整的 Web 可视化系统。系统采用 C++ JSON Exporter + Web Visualizer 的分离架构，支持追踪执行过程中每个阶段的图状态变化，并使用多层高亮方案显示节点/边的动态变化。

### 🔍 Technical Details

- **JSON Schema**: 定义了 Stage/Sub-Step/GraphData/Highlights/StateInfo 等核心数据结构
- **DOT 存储**: 采用完整 DOT 存储方案 (方案 A)，每个步骤独立保存完整图状态
- **多层高亮**: 节点类型(圆形/方形) → 胜利区域(绿/红) → 动态变化(新增/SCC/待处理)
- **输出位置**: `results/trace_{timestamp}/HH-period/formula_id/trace.json`
- **前端技术栈**: Vite + Vue 3 + TypeScript + @viz-js/viz

### 📊 Impact Analysis

- 新增文档仅涉及设计规范，不影响现有代码
- 为后续 Phase 1-4 的实现提供明确的技术规范
- 支持离线查看 trace (单个 HTML 文件包含完整数据)

## Changes

### Added
- `docs/TRACE_VISUALIZATION/implementation_plan.md`
- `docs/TRACE_VISUALIZATION/json_schema.md`
- `docs/TRACE_VISUALIZATION/README.md`


## Stats

- **3** files changed
- **1221** insertions(+)
- **0** deletions(-)
