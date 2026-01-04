# E3 分类不一致问题

**日期**: 2026-01-04 下午
**状态**: Open
**优先级**: High

---

## 问题描述

在 trace 文件 `trace_20260104_165731.json` 的最终 stage (stage_002) 中：

| 来源 | E3 分类 |
|------|---------|
| **DOT** | `Ewin` (`fillcolor=lightcoral, label="E3\\nEwin"`) |
| **state_data** | `Swin` (`"classification": "Swin"`) |

## 症状

前端 UI 显示 E3 为 Ewin（因为前端渲染的是 DOT），但 state_data 中 E3 是 Swin（tooltip 显示正确）。

## 分析

### 调用链

```cpp
// trace_exporter.cpp:finalize()
std::string dot = solver.to_dot();           // ← DOT 生成
collect_state_data(graph_data, solver);      // ← state_data 收集
```

两个调用使用同一个 `solver` 实例，应该访问相同的 `classification_` 数据。

### 可能原因

1. **两个不同的 StateIdMap 实例**：
   - `to_dot()` 使用本地 `StateIdMap id_map`
   - `collect_state_data()` 使用 `TraceExporter::id_map_`
   - `successors_` 是 `unordered_map`，遍历顺序不确定

2. **GameState 比较问题**：
   - `StateIdMap` 使用 `GameState` 作为 key
   - `GameState` 的 `operator==` 或 hash 函数可能有问题
   - 导致两个不同的 GameState 被认为是同一个

3. **classification_ 更新时序**：
   - `to_dot()` 和 `collect_state_data()` 之间 `classification_` 发生了变化

## 调试结果

### 添加日志后的发现

通过添加调试日志，发现了根本原因：

```
[DEBUG collect_state_data] node=E3, cls=Swin, phi=true
[DEBUG to_dot] node=E3, cls=Ewin, phi=(true U p)
```

**存在两个不同的 E3 状态**：
1. phi=true, classification=Swin
2. phi=(true U p), classification=Ewin

但它们都被分配了相同的 ID "E3"！

### 根本原因

1. **两个不同的 StateIdMap 实例**：
   - `to_dot()` 使用本地 `StateIdMap`（每次调用都重新创建）
   - `collect_state_data()` 使用 `TraceExporter::id_map_`（持久化）

2. **successors_ 包含多个 E3 状态**：
   - 在某个阶段，phi=true 的 E3 被添加到 successors_
   - 在另一个阶段，phi=(true U p) 的 E3 也被添加

3. **ID 映射不一致**：
   - `TraceExporter::id_map_` 先遇到 phi=true 的 E3，分配 ID "E3"
   - `to_dot()` 的本地 `StateIdMap` 可能先遇到 phi=(true U p) 的 E3，也分配 ID "E3"

### 待确认的问题

1. **为什么 successors_ 中有两个不同的 E3 状态？**
   - 是状态扩展逻辑的问题吗？
   - 还是 GameState 比较逻辑的问题？

2. **为什么两个 E3 状态被分配了相同的 ID？**
   - GameState 的 operator== 或 hash 函数是否正确？
   - 是两个确实不同的 GameState 对象被错误地认为相同？

## 相关文件

- `src/synthesis/game_graph_export.cpp`: `to_dot()`
- `src/synthesis/trace_exporter.cpp`: `collect_state_data()`, `finalize()`
- `src/synthesis/on_the_fly_solver.cpp`: `classification_`, `GameState`
