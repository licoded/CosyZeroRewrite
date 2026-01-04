# Trace Visualization 实现计划

> 版本: 1.0
> 日期: 2026-01-04

---

## Phase 1: C++ Trace Exporter (后端)

### 1.1 创建 TraceExporter 类

**文件**: `include/synthesis/trace_exporter.hpp`

```cpp
namespace synthesis {

class TraceExporter {
public:
    explicit TraceExporter(
        const std::string& formula,
        const std::filesystem::path& output_dir
    );

    ~TraceExporter();

    // 阶段管理
    void begin_stage(const std::string& stage_type,
                     const std::string& description);
    void end_stage();

    // 子步骤管理
    void begin_sub_step(const std::string& description);
    void end_sub_step();

    // 数据记录
    void set_graph_dot(const std::string& dot);
    void add_highlight_node(const std::string& node_id,
                            HighlightType type);
    void add_highlight_edge(const std::string& from,
                            const std::string& to,
                            const std::string& label);
    void set_state_info(int swin, int ewin, int unknown);

    // 完成
    void finalize(bool realizable);

private:
    // ...
};

} // namespace synthesis
```

### 1.2 集成到 OnTheFlyGameSolver

**文件**: `src/synthesis/on_the_fly_game_solver.cpp`

```cpp
class OnTheFlyGameSolver {
private:
    std::unique_ptr<TraceExporter> trace_exporter_;

public:
    bool solve(const Partition& partition) {
        // 初始化 trace exporter
        if (config_.trace_enabled) {
            trace_exporter_ = std::make_unique<TraceExporter>(
                phi_->to_string_with_names(pool_),
                config_.trace_output_dir
            );
        }

        // Expand stage
        trace_exporter_->begin_stage("expand", "State expansion");
        expand_states();
        trace_exporter_->end_stage();

        // SCC stage
        trace_exporter_->begin_stage("scc", "SCC detection");
        find_sccs();
        trace_exporter_->end_stage();

        // ...
    }
};
```

### 1.3 DOT 生成集成

利用现有的 `game_graph_export.cpp` 中生成 DOT 的逻辑：

```cpp
// 在 TraceExporter 中
void TraceExporter::capture_game_graph_state(const OnTheFlyGameSolver& solver) {
    std::ostringstream dot;
    solver.to_dot(dot);  // 复用现有逻辑

    current_sub_step_.graph_data.dot = dot.str();
    current_sub_step_.graph_data.num_nodes = solver.num_states();
    current_sub_step_.graph_data.num_edges = solver.num_transitions();
}
```

### 1.4 任务清单

- [ ] 创建 `include/synthesis/trace_exporter.hpp`
- [ ] 创建 `src/synthesis/trace_exporter.cpp`
- [ ] 实现 JSON 序列化 (使用 nlohmann/json 或手写)
- [ ] 在 `OnTheFlyGameSolver` 中添加 trace 点
- [ ] 在 `game_graph_export` 中添加 DOT 生成接口
- [ ] 添加配置选项 (`--trace` 命令行参数)
- [ ] 单元测试

---

## Phase 2: 前端基础

### 2.1 项目初始化

```bash
# 在 web/ 目录下
npm create vite@latest trace-visualizer -- --template vue-ts
cd trace-visualizer
npm install
```

### 2.2 依赖安装

```bash
# Graphviz 渲染
npm install @viz-js/viz

# UI 组件库 (可选)
npm install element-plus
# 或
npm install shadcn-vue

# 状态管理
npm install pinia
```

### 2.3 项目结构

```
web/trace-visualizer/
├── index.html
├── src/
│   ├── main.ts
│   ├── App.vue
│   ├── components/
│   │   ├── StepTree.vue      # 左侧步骤树
│   │   ├── GraphCanvas.vue   # 主画布
│   │   ├── Stepper.vue       # 步进控制器
│   │   └── HighlightLegend.vue  # 图例
│   ├── composables/
│   │   ├── useTrace.ts       # Trace 数据加载
│   │   └── useGraphViz.ts    # DOT 渲染
│   ├── types/
│   │   └── trace.ts          # TypeScript 类型定义
│   └── assets/
│       └── styles/
│           └── main.css
└── vite.config.ts
```

### 2.4 任务清单

- [ ] 初始化 Vite + Vue 3 + TypeScript 项目
- [ ] 安装依赖 (@viz-js/viz, pinia)
- [ ] 创建基础布局 (三栏: Sidebar, Canvas, Controls)
- [ ] 实现文件加载器 (JSON trace 文件)
- [ ] 实现 DOT 渲染 (使用 @viz-js/viz)

---

## Phase 3: 交互功能

### 3.1 StepTree 组件

```vue
<!-- StepTree.vue -->
<template>
  <div class="step-tree">
    <div v-for="stage in trace.stages" :key="stage.stage_id">
      <div class="stage-header" @click="toggleStage(stage.stage_id)">
        <span>{{ stage.stage_type }}</span>
        <span>{{ stage.sub_steps.length }} steps</span>
      </div>
      <div v-if="expandedStages.has(stage.stage_id)" class="sub-steps">
        <div
          v-for="step in stage.sub_steps"
          :key="step.step_id"
          :class="['step-item', { active: isActive(step) }]"
          @click="selectStep(step)"
        >
          {{ step.description }}
        </div>
      </div>
    </div>
  </div>
</template>
```

### 3.2 GraphCanvas 组件

```vue
<!-- GraphCanvas.vue -->
<template>
  <div class="graph-canvas" ref="container">
    <svg ref="svgElement" v-html="svgContent"></svg>
    <canvas v-if="highlightsEnabled" ref="overlayCanvas"></canvas>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, computed } from 'vue';
import { useGraphViz } from '@/composables/useGraphViz';

const props = defineProps<{
  dot: string;
  highlights?: Highlights;
}>();

const { renderDot, applyHighlights } = useGraphViz();
const svgContent = ref('');

watch(() => props.dot, async (newDot) => {
  svgContent.value = await renderDot(newDot);
  if (props.highlights) {
    applyHighlights(props.highlights);
  }
}, { immediate: true });
</script>
```

### 3.3 Stepper 组件

```vue
<!-- Stepper.vue -->
<template>
  <div class="stepper">
    <button @click="prev" :disabled="currentStepIndex <= 0">
      ← Previous
    </button>
    <span>Step: {{ currentStepIndex + 1 }} / {{ totalSteps }}</span>
    <button @click="next" :disabled="currentStepIndex >= totalSteps - 1">
      Next →
    </button>
    <button @click="toggleAutoPlay">
      {{ autoPlaying ? '❚❚ Pause' : '▶ Auto' }}
    </button>
    <input type="range" v-model="speed" min="100" max="2000" step="100" />
  </div>
</template>
```

### 3.4 任务清单

- [ ] 实现 StepTree 组件 (展开/折叠)
- [ ] 实现 GraphCanvas 组件 (SVG 渲染)
- [ ] 实现 Stepper 组件 (前进/后退/自动播放)
- [ ] 实现高亮逻辑 (多层叠加)
- [ ] 添加键盘快捷键 (←/→ 箭头)

---

## Phase 4: 优化与集成

### 4.1 性能优化

**虚拟滚动** (StepTree):
- 大量步骤时使用虚拟滚动

**SVG 优化**:
- 使用 `will-change` 提示浏览器优化
- 大图时简化渲染细节

**懒加载**:
- 只渲染当前可见步骤的 DOT

### 4.2 主程序集成

**命令行参数**:
```cpp
// src/main.cpp
args::ArgumentFlag trace_flag(parser, "trace", "Enable trace visualization");
args::ValueFlag<std::string> trace_dir(parser, "dir", "Trace output directory", {"trace-dir"});
```

**输出位置**:
```cpp
// 自动生成目录结构
auto trace_path = get_trace_output_path();
// results/trace_20260104_120000/01-morning/formula_abc123/trace.json
```

### 4.3 任务清单

- [ ] 性能优化 (虚拟滚动, 懒加载)
- [ ] 与主程序集成 (--trace 参数)
- [ ] 添加单元测试
- [ ] 用户文档

---

## 开发顺序

```
Phase 1 (后端)                    Phase 2 (前端基础)
    │                                   │
    ├── 1.1 TraceExporter 类            ├── 2.1 项目初始化
    ├── 1.2 集成到 Solver               ├── 2.2 基础布局
    ├── 1.3 DOT 生成                    ├── 2.3 DOT 渲染
    └── 1.4 配置选项                    └── 2.4 文件加载
                │                              │
                └──────────────┬───────────────┘
                               ▼
                    Phase 3 (交互功能)
                               │
                    ├── 3.1 StepTree
                    ├── 3.2 GraphCanvas
                    ├── 3.3 Stepper
                    └── 3.4 高亮逻辑
                               │
                               ▼
                    Phase 4 (优化集成)
                               │
                    ├── 4.1 性能优化
                    ├── 4.2 主程序集成
                    └── 4.3 文档测试
```

---

## 验收标准

### Phase 1
- [ ] 可以生成包含完整 trace 信息的 JSON 文件
- [ ] JSON 符合 schema 定义
- [ ] DOT 字符串可正确渲染

### Phase 2
- [ ] 可以加载和解析 JSON trace 文件
- [ ] DOT 可以正确渲染为 SVG
- [ ] 布局响应式适配

### Phase 3
- [ ] 可以按步骤浏览整个执行过程
- [ ] 高亮正确显示变化
- [ ] 自动播放功能正常

### Phase 4
- [ ] 大规模 trace (>100 步) 流畅运行
- [ ] 通过 `--trace` 参数启用
- [ ] 用户文档完整
