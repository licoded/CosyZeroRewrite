# Tarjan SCC 算法详解

> 强连通分量分解算法的学习记录和实现分析

---

## 1. 基本概念

### 1.1 强连通分量（SCC）

**定义**：有向图 G 的极大强连通子图

**强连通**：G 中任意两个结点互相可达

```
示例图：

    1 → 2 → 3 ← 4
    ↑       │   │
    └───────┘   ↓
                5

SCC 分解：
- SCC1: {1, 2, 3} （互相可达）
- SCC2: {4}
- SCC3: {5}
```

### 1.2 DFS 生成树的 4 种边

| 类型 | 说明 | 图例 |
|------|------|------|
| **树边** | 搜索到未访问节点 | 黑色边 |
| **反祖边/回边** | 指向祖先 | 红色边 (7→1) |
| **横叉边** | 指向已访问但非祖先 | 蓝色边 (9→7) |
| **前向边** | 指向子树中节点 | 绿色边 (3→6) |

---

## 2. Tarjan 算法

### 2.1 核心思想

**关键洞察**：如果 u 是某个 SCC 在 DFS 树中第一个被访问的节点，那么该 SCC 的其余节点都在以 u 为根的子树中。

**SCC 根的判定**：`dfn[u] == low[u]`

### 2.2 变量定义

| 变量 | 含义 | 初始化 |
|------|------|--------|
| `dfn[u]` | 节点 u 的访问时间戳 | 访问时递增 |
| `low[u]` | u 子树能回溯到的栈中最早节点的 dfn | 初始化为 dfn[u] |
| `stack` | 保存当前 DFS 路径 | 访问时 push |
| `on_stack[u]` | u 是否在栈中 | push 时设为 true |

### 2.3 算法流程

```
TARJAN_SEARCH(u):
    dfn[u] = low[u] = ++dfncnt
    push u to stack
    on_stack[u] = true

    for each edge (u, v):
        if dfn[v] == 0:              // v 未访问
            TARJAN_SEARCH(v)
            low[u] = min(low[u], low[v])
        else if on_stack[v]:         // v 在栈中
            low[u] = min(low[u], dfn[v])
        // else: v 已访问但不在栈中，属于其他 SCC，忽略

    if dfn[u] == low[u]:             // u 是 SCC 根
        弹出栈中元素直到 u，构成一个 SCC
```

### 2.4 代码实现（标准模板）

```cpp
int dfn[N], low[N], dfncnt;
int stack[N], top;
bool in_stack[N];
int scc_id[N];  // 节点所属的 SCC 编号
int scc_cnt;    // SCC 总数

void tarjan(int u) {
    low[u] = dfn[u] = ++dfncnt;
    stack[++top] = u;
    in_stack[u] = true;

    for (int i = head[u]; i; i = e[i].next) {
        int v = e[i].to;
        if (!dfn[v]) {
            tarjan(v);
            low[u] = std::min(low[u], low[v]);
        } else if (in_stack[v]) {
            low[u] = std::min(low[u], dfn[v]);
        }
    }

    if (dfn[u] == low[u]) {
        ++scc_cnt;
        int v;
        do {
            v = stack[top--];
            in_stack[v] = false;
            scc_id[v] = scc_cnt;
        } while (v != u);
    }
}
```

### 2.5 复杂度分析

- **时间复杂度**：O(V + E)，每个节点和边只处理一次
- **空间复杂度**：O(V)，存储 dfn、low、栈等

---

## 3. 三种 SCC 算法对比

### 3.1 Tarjan 算法

| 特点 | 说明 |
|------|------|
| DFS 次数 | 1 次 |
| 数据结构 | dfn/low + 栈 |
| 优点 | 单次 DFS，效率高 |
| 缺点 | 实现稍复杂 |

### 3.2 Kosaraju 算法

| 特点 | 说明 |
|------|------|
| DFS 次数 | 2 次 |
| 数据结构 | 原图 + 反图 |
| 流程 | 1. 原图 DFS 得到后序<br>2. 反图按逆后序 DFS |
| 优点 | 思路简单直观 |
| 缺点 | 需要建反图，两次遍历 |

### 3.3 Garbow 算法

| 特点 | 说明 |
|------|------|
| DFS 次数 | 1 次 |
| 数据结构 | 双栈 |
| 特点 | Tarjan 的变体，用第二个栈判断 SCC 边界 |

---

## 4. SCC 与拓扑序

**重要性质**：Tarjan 算法发现的 SCC 顺序是 **逆拓扑序**

```
原图：SCC1 → SCC2 → SCC3
Tarjan 发现顺序：SCC3, SCC2, SCC1（逆拓扑序）

原因：算法先处理没有出边的 SCC（汇点），然后向上回溯
```

**应用**：
- 缩点后得到 DAG
- 可按逆拓扑序处理 SCC（后继先处理）
- 适合动态规划类问题

---

## 5. 当前实现分析

### 5.1 代码位置

`src/synthesis/on_the_fly_solver.cpp::find_sccs()`

### 5.2 问题分析

**问题 1：每次都全量重新计算**

```cpp
// 当前实现
while (!worklist_.empty() && !is_initial_classified()) {
    expand_state(state);
    auto sccs = find_sccs();  // ← 每次都全量计算！
    ...
}
```

- 每次 expand 后，对**整个**图重新做 SCC 分解
- 时间复杂度：O(次数 × V × E)
- 没有利用之前计算的结果

**问题 2：非真正的 On-The-Fly**

- 真正的 on-the-fly SCC 算法（如 Pearce 算法）可以增量更新
- 当前实现是批处理：expand → 完全分解 → 分类

**问题 3：设计合理性**

用户的质疑：
> 不是每次 expand 后都能确定新的 SCC

这是对的！例如：
- 只增加一个节点和一条边
- 可能只是扩展了某个现有 SCC
- 也可能创建了新的独立 SCC

### 5.3 当前实现的正确性

**好消息**：算法实现本身是正确的 Tarjan

```cpp
std::function<void(const GameState&)> strongconnect = [&](const GameState& v) {
    indices[v] = index;
    lowlinks[v] = index;
    index++;
    stack.push_back(v);
    on_stack[v] = true;

    for (const GameState& w : succs) {
        if (indices.count(w) == 0) {          // 未访问
            strongconnect(w);
            lowlinks[v] = std::min(lowlinks[v], lowlinks[w]);
        } else if (on_stack[w]) {             // 在栈中
            lowlinks[v] = std::min(lowlinks[v], indices[w]);
        }
        // 已访问但不在栈中：属于其他 SCC，忽略 ✓
    }

    if (lowlinks[v] == indices[v]) {         // SCC 根
        // 弹出栈中元素构成 SCC
    }
};
```

**结论**：代码逻辑正确，但效率不高。

---

## 6. 可能的优化方向

### 6.1 增量 SCC 算法

- **Pearce 算法**：支持增量更新的 SCC 算法
- **复杂度**：理论上可以做到 O(新边数) 的更新
- **实现难度**：较高

### 6.2 当前方案的优化

即使保持批处理，也可以优化：

```cpp
// 优化思路：缓存 + 按需计算
bool need_recompute = false;
for (const auto& new_state : newly_expanded) {
    if (可能影响现有 SCC) {
        need_recompute = true;
        break;
    }
}

if (need_recompute) {
    sccs = find_sccs();
}
```

### 6.3 问题优先级

根据 on-the-fly 优化的整体目标：
1. 首先实现逐步展开 + 剪枝（更大的收益）
2. SCC 增量更新作为后续优化

---

## 7. 决策记录

### 7.1 当前状态

**实现位置**：`src/synthesis/on_the_fly_solver.cpp::find_sccs()`

**评估结论**：
- ✅ 代码逻辑正确（标准 Tarjan 实现）
- ❌ 效率不高（每次 expand 后全量重新计算）
- ❌ 不符合 on-the-fly 理念

### 7.2 未来方案：增量 SCC 算法（方案 B）

**决策**：采用增量 SCC 算法作为长期优化方向

**候选算法**：

| 算法 | 特点 | 复杂度 |
|------|------|--------|
| **Pearce 算法** | 专利算法，支持增量更新，Python NetworkX 使用 | O(V + E) |
| **Habib 算法** | 基于DFS的增量维护 | O(V + E) |
| **Bender 算法** | 动态维护 SCC 分解 | O(log² V) 摊销 |

**Pearce 算法伪代码**（参考）：
```
PEARCE_SCC():
    for v in vertices:
        if not v.visited:
            PEARCE_VISIT(v)

PEARCE_VISIT(v):
    v.root = true
    v.visited = true
    stack.push(v)
    for w in v.out_edges:
        if not w.visited:
            PEARCE_VISIT(w)
        elif w.root:
            # 边到当前 SCC 的根
            start_component(w)

    if v.root:
        start_component(v)

START_COMPONENT(v):
    # 标记当前 SCC
    c = stack.top
    while c != v:
        c.root = false
        c = c.prev
    # 处理 SCC
```

**实现计划**：
1. 当前阶段：保留实现，充分测试
2. 中期：实现 on-the-fly 优化（逐步展开 + 剪枝）
3. 长期：替换为增量 SCC 算法

### 7.3 测试策略

**需要覆盖的场景**：

| 场景 | 描述 | 预期 |
|------|------|------|
| 单节点 | 只有一个状态 | 1 个 SCC |
| 线性链 | a→b→c→d | 4 个 SCC |
| 简单环 | a→b→c→a | 1 个 SCC |
| 双环连接 | a↔b, c↔d, b→c | 2 个 SCC |
| 自环 | a→a | 1 个 SCC {a} |
| 复杂 DAG | 多层依赖 | 每个节点独立 SCC |
| 十字交叉 | a→b, c→d, a→c, b→d | 4 个 SCC |

---

## 8. 参考资料

- [OI Wiki - 强连通分量](https://oi-wiki.org/graph/scc/)
- [Wikipedia - Tarjan's SCC algorithm](https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm)
- [CP-Algorithms - Strongly Connected Components](https://cp-algorithms.com/graph/strongly-connected-components.html)
- Tarjan, R. (1972). "Depth-first search and linear graph algorithms". SIAM Journal on Computing.
