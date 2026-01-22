# TraceExporter 重构设计文档

**日期**: 2026-01-06 AM
**问题**: trace_exporter.cpp 中的前置检查代码重复 (8 处，约 50 行)

---

## 问题分析

### 当前重复模式

每个设置方法都有相同的前置检查代码：

```cpp
void TraceExporter::set_scc_id(const std::string& scc_id) {
    if (!enabled_ || current_stage_index_ < 0) return;        // 检查 1
    TraceStage& stage = stages_[current_stage_index_];
    if (stage.sub_steps.empty()) return;                       // 检查 2
    SubStep& step = stage.sub_steps.back();                    // 获取引用
    step.highlights.scc_id = scc_id;                           // 实际操作
}
```

### 受影响的方法 (8+ 个)

1. `add_highlight_edge()` - 行 321-327
2. `set_state_info()` - 行 330-340
3. `set_scc_id()` - 行 343-350
4. `set_graph_dot()` - 行 269-277
5. `add_highlight_node()` - 行 282-288
6. `capture_state()` 中的子逻辑 - 行 369-371
7. `record_expansion()` 中的子逻辑 - 行 424-426
8. `record_scc()` 中的子逻辑 - 行 469-471
9. `record_classification_change()` 中的子逻辑 - 行 513-515

---

## 设计方案：缓存指针

### 核心思想

将检查和 SubStep 获取操作移到 `begin_sub_step()` 中，缓存当前 SubStep 的指针。后续方法直接使用缓存指针，无需重复检查。

### 架构图

```
┌─────────────────────────────────────────────────────────────┐
│  begin_sub_step()                                          │
│  ├─ 检查 enabled_                                         │
│  ├─ 检查 current_stage_index_                             │
│  ├─ 创建新 SubStep                                          │
│  └─ 缓存指针: current_sub_step_ = &step ◄── 只检查一次！     │
├─────────────────────────────────────────────────────────────┤
│  期间的操作 (set_scc_id, add_highlight_node, ...)           │
│  └─ 直接使用 current_sub_step_                              │
│      if (current_sub_step_) {                              │
│          current_sub_step_->xxx = ...;  // 直接指针访问      │
│      }                                                       │
├─────────────────────────────────────────────────────────────┤
│  end_sub_step()                                              │
│  └─ 清空指针: current_sub_step_ = nullptr                    │
└─────────────────────────────────────────────────────────────┘
```

---

## 实现计划

### 1. 修改头文件 (trace_exporter.hpp)

添加新的私有成员变量：

```cpp
class TraceExporter {
private:
    // ... 其他成员 ...

    /**
     * @brief Cached pointer to the current active SubStep
     *
     * This pointer is set by begin_sub_step() and cleared by end_sub_step().
     * All intermediate operations can use this pointer directly without
     * repeatedly checking enabled_, current_stage_index_, and sub_steps.
     *
     * nullptr means no active sub_step (either disabled, not in a stage,
     * or not between begin/end_sub_step calls).
     */
    SubStep* current_sub_step_ = nullptr;
};
```

### 2. 修改 begin_sub_step()

```cpp
void TraceExporter::begin_sub_step(const std::string& description = "") {
    // ========== 只在这里做检查 ==========
    if (!enabled_) return;
    if (current_stage_index_ < 0) return;

    TraceStage& stage = stages_[current_stage_index_];

    // 创建新 SubStep
    stage.sub_steps.emplace_back();
    SubStep& step = stage.sub_steps.back();
    step.step_id = generate_step_id();
    step.description = description;
    step.metrics.duration_ms = 0;

    // ========== 缓存指针 ==========
    current_sub_step_ = &step;
    step_start_time_ = std::chrono::steady_clock::now();
}
```

### 3. 修改 end_sub_step()

```cpp
void TraceExporter::end_sub_step(bool include_graph = true) {
    if (!enabled_) return;

    // 使用缓存指针记录时间
    if (current_sub_step_) {
        auto end_time = std::chrono::steady_clock::now();
        current_sub_step_->metrics.duration_ms =
            std::chrono::duration<double, std::milli>(
                end_time - step_start_time_).count();
    }

    // ========== 清空指针 ==========
    current_sub_step_ = nullptr;

    // Log
    LOG_DEBUG("TraceExporter: ended step ", step_counter_,
              " (", stage.sub_steps.back().description, "), duration: ",
              stage.sub_steps.back().metrics.duration_ms, "ms");
}
```

### 4. 简化各个设置方法

```cpp
void TraceExporter::set_scc_id(const std::string& scc_id) {
    if (current_sub_step_) {
        current_sub_step_->highlights.scc_id = scc_id;
    }
}

void TraceExporter::set_state_info(int swin, int ewin, int unknown, int total) {
    if (current_sub_step_) {
        auto& info = current_sub_step_->state_info;
        info.swin_count = swin;
        info.ewin_count = ewin;
        info.unknown_count = unknown;
        info.total_states = (total >= 0) ? total : (swin + ewin + unknown);
    }
}

void TraceExporter::add_highlight_edge(const std::string& from,
                                       const std::string& to,
                                       const std::string& label,
                                       const std::string& type) {
    if (current_sub_step_) {
        current_sub_step_->highlights.new_edges.emplace_back(from, to, label, type);
    }
}

void TraceExporter::set_graph_dot(const std::string& dot,
                                   size_t num_nodes,
                                   size_t num_edges) {
    if (current_sub_step_) {
        current_sub_step_->graph_data.dot = dot;
        current_sub_step_->graph_data.num_nodes = num_nodes;
        current_sub_step_->graph_data.num_edges = num_edges;
    }
}
```

### 5. 简化 capture_state 等方法中的子逻辑

原来：
```cpp
TraceStage& stage = stages_[current_stage_index_];
if (!stage.sub_steps.empty()) {
    collect_state_data(stage.sub_steps.back().graph_data, solver);
}
```

改为：
```cpp
if (current_sub_step_) {
    collect_state_data(current_sub_step_->graph_data, solver);
}
```

---

## 优势总结

| 维度 | 原方案 | 新方案 |
|------|--------|--------|
| 每次调用检查次数 | 3 个条件 | 1 个指针检查 |
| 代码行数 (每个方法) | 6-8 行 | 2-3 行 |
| 总减少代码量 | ~50 行 | ~50 行 |
| 运行时性能 | 每次访问 vector | 直接指针访问 |
| 可维护性 | 分散检查 | 集中在 begin/end |

---

## 测试计划

1. 编译测试
2. 运行现有单元测试
3. 运行 synthesis_test（使用 TraceExporter）
4. 检查 jscpd 重复率是否降低

---

## 状态

- [x] 设计文档完成
- [x] 修改头文件
- [x] 修改 begin_sub_step
- [x] 修改 end_sub_step
- [x] 简化各个设置方法
- [x] 编译测试
- [x] 运行单元测试
- [x] 运行 jscpd 验证
- [x] 提交 git (ff4c519)

## 实际结果

### 代码重复指标对比

| 指标 | 重构前 | 重构后 | 改善 |
|------|--------|--------|------|
| 克隆数量 | 19 | 15 | -21% |
| 重复行数 | 192 (2.22%) | 151 (1.75%) | -27% |
| 重复 token | 2325 (3.3%) | 1686 (2.4%) | -27% |

### trace_exporter.cpp 变化

- **消除的重复模式**: 前置检查（`if (!enabled_ || current_stage_index_ < 0)` 等）
- **新增成员**: `SubStep* current_sub_step_ = nullptr;`
- **影响的方法**: 8+ 个方法简化为单指针检查模式
