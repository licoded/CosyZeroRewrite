# Debug Guide

> Synthesis 调试方法

---

## TraceExporter 可视化

TraceExporter 是 synthesis 模块内置的执行追踪系统，用于可视化调试。

### 功能

1. **JSON 格式导出** - 包含完整的执行过程信息
2. **DOT 格式导出** - 用于 Graphviz 可视化
3. **Vue Web 界面** - 专门的 web 系统解析 JSON 进行交互式可视化

### 使用方法

在 `on_the_fly_solver.cpp` 中已集成，通过命令行参数启用：

```bash
# 启用 trace 输出
./build/output/Cosy2 -f formula.ltlf -p part.part --trace output_dir

# 输出文件
# output_dir/trace_YYYYMMDD_HHMMSS.json  - JSON 格式（供 Vue 系统使用）
# output_dir/trace_YYYYMMDD_HHMMSS.dot   - DOT 格式（供 Graphviz 使用）
```

### JSON 内容

```json
{
  "formula": "原始公式字符串",
  "partition": {"inputs": [...], "outputs": [...]},
  "stages": [
    {
      "name": "expand",
      "states": [...],
      "edges": [...]
    },
    {
      "name": "scc",
      "sccs": [...],
      "highlights": [...]
    }
  ],
  "result": "REALIZABLE/UNREALIZABLE"
}
```

### Vue Web 界面

1. 打开 `output_dir/trace_*.html`（自动生成）
2. 交互式浏览执行过程：
   - 状态节点详情
   - SCC 分解过程
   - 边转换关系
   - 分类结果高亮

---

## 图例

| 形状 | 含义 |
|------|------|
| `box` | System 状态 |
| `ellipse` | Environment 状态 |

| 颜色 | 含义 |
|------|------|
| `green` | accepting 状态 |
| `red` | non-accepting 状态 |
| `blue` | terminal 状态 |

---

## DOT 可视化工具

- **在线**: https://dreampuf.github.io/GraphvizOnline/
- **本地**: `dot -Tpng game.dot -o game.png`
