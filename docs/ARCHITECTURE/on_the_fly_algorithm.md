# On-The-Fly Synthesis 算法详解

> `is_realizable()` 核心循环的完整算法逻辑

---

## ⚠️ 重要说明

本文档记录了 **正确的算法理解**，之前的文档存在严重错误（见附录）。

---

## 1. 算法概述

### 1.1 什么是 On-The-Fly？

**传统算法** vs **On-The-Fly**：

| 方面 | 传统算法 | On-The-Fly 算法 |
|------|---------|-----------------|
| 图构建 | 预先构建完整游戏图 | 按需展开状态 |
| 内存 | 需要存储所有状态 | 只存储可达状态 |
| 效率 | 可能展开无用状态 | 只展开必要状态 |
| 适用场景 | 小规模公式 | 大规模公式 |

### 1.2 核心思想

```
┌─────────────────────────────────────────────────────────┐
│                  On-The-Fly 循环                        │
├─────────────────────────────────────────────────────────┤
│  while (有未处理状态 && 初始状态未分类) {                │
│                                                         │
│      1. EXPAND:   展开一个状态，计算后继                │
│                                                         │
│      2. SCC_DECOMPOSE: 对当前图做强连通分解             │
│                                                         │
│      3. SOLVE_SCC:  用不动点算法求解 SCC 内部状态       │
│                                                         │
│      4. PROPAGATE: 向后传播分类结果                     │
│                                                         │
│      5. ENQUEUE:   未分类的后继加入 worklist            │
│  }                                                      │
│                                                         │
│  return (初始状态 == Swin);                            │
└─────────────────────────────────────────────────────────┘
```

---

## 2. 基本概念

### 2.1 状态定义

| 状态类型 | 该谁移动 | 后继是什么 | 代码位置 |
|---------|---------|-----------|---------|
| **System** | System 选择输出 | Environment 状态（每个输出一个） | `on_the_fly_solver.cpp:237-238` |
| **Environment** | Environment 选择输入 | System 状态（每个输入一个，DFA 状态更新） | `on_the_fly_solver.cpp:259` |

### 2.2 状态转移链

```
System 状态 (选择输出)
    │
    ↓ output ∈ Outputs
    │
Environment 状态 (记录 System 选的输出)
    │
    ↓ input ∈ Inputs
    │
System 状态 (选择输出，DFA 状态更新)
    │
    ... 循环
```

---

## 3. SCC 的真正意义

### 3.1 为什么需要 SCC？

**问题**：状态分类存在循环依赖

```
q₀ 的胜负依赖 q₁
q₁ 的胜负依赖 q₂
q₂ 的胜负依赖 q₀  ← 循环依赖！
```

**解决**：SCC 分解得到拓扑序

```
SCC 之间形成 DAG（无环）：
    SCC_A → SCC_B → SCC_C

按拓扑序遍历：
    - 处理 SCC_A 时（它的后继 SCC 还没处理）
    - 处理 SCC_B 时（SCC_A 已处理）
    - 处理 SCC_C 时（SCC_A, SCC_B 已处理）

关键：处理某个 SCC 时，它的后继 SCC 都已经处理完了！
```

### 3.2 SCC 的作用

| 作用 | 说明 |
|------|------|
| **拓扑排序** | SCC 之间形成 DAG，可以按拓扑序处理 |
| **解决循环依赖** | 后继 SCC 先处理，保证后继状态已分类 |
| **隔离循环** | 唯一的循环依赖是 SCC **内部**状态之间 |

### 3.3 ❌ 错误理解

> **错误**："SCC 内所有状态具有相同胜负状态"
>
> **正确**：SCC 内状态需要分别判断，通过不动点迭代

---

## 4. SCC 内部的不动点算法

### 4.1 算法流程

```
┌─────────────────────────────────────────────────────────┐
│           SCC 内部状态分类（不动点迭代）                 │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. 初始化种子集合：                                     │
│     - 接受状态（DFA accepting state）                   │
│     - true 公式状态                                     │
│     - End 公式状态                                      │
│     → 这些状态初始标记为 Swin                           │
│                                                         │
│  2. 迭代循环：                                           │
│     while (这一轮有新的 Swin 产生) {                    │
│                                                         │
│         a. 找所有当前 Swin 状态的前继 → new_set         │
│                                                         │
│         b. 在 new_set 中用传播规则判断新的 Swin：       │
│            - System 状态：任一后继是 Swin → Swin       │
│            - Environment 状态：所有后继是 Swin → Swin  │
│                                                         │
│         c. 把新判断出的 Swin 加入种子集合               │
│     }                                                    │
│                                                         │
│  3. 终止条件：                                           │
│     某一轮迭代没有产生新的 Swin                         │
│                                                         │
│  4. 剩余状态分类：                                       │
│     未被标记为 Swin 的状态 → 全部标记为 Ewin            │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 4.2 为什么这样设计？

**直觉**：
- Swin 是"优势"，会向前传播（从后继到前驱）
- Ewin 是"劣势"，是"不是 Swin"的默认状态
- 先找出所有能确定是 Swin 的状态
- 剩下的无法证明是 Swin → 就是 Ewin

**为什么迭代到不动点**：
- 第 1 轮：种子（接受状态）的直连前继变成 Swin
- 第 2 轮：新 Swin 的前继变成 Swin
- ...
- 第 k 轮：没有新的 Swin 产生 → 停止

### 4.3 伪代码

```cpp
void solve_scc(const vector<GameState>& scc) {
    // 1. 初始化：找出所有接受状态作为 Swin 种子
    unordered_set<GameState> swin_states;
    for (const auto& s : scc) {
        if (is_accepting(s.dfa_state) || is_true_or_end(s)) {
            swin_states.insert(s);
        }
    }

    // 2. 不动点迭代
    bool changed = true;
    while (changed) {
        changed = false;
        unordered_set<GameState> new_swin_states;

        // 找所有当前 Swin 状态的前继
        for (const GameState& swin : swin_states) {
            for (const GameState& pred : get_predecessors(swin)) {
                if (scc.count(pred) && !swin_states.count(pred)) {
                    // 判断前继是否是 Swin
                    if (is_swin(pred, swin_states)) {
                        new_swin_states.insert(pred);
                        changed = true;
                    }
                }
            }
        }

        swin_states.insert(new_swin_states.begin(), new_swin_states.end());
    }

    // 3. 剩余状态标记为 Ewin
    for (const auto& s : scc) {
        if (!swin_states.count(s)) {
            classification_[s] = StateClass::Ewin;
        } else {
            classification_[s] = StateClass::Swin;
        }
    }
}
```

---

## 5. 正确的传播规则

### 5.1 核心逻辑

| 当前状态 | 条件 | 结果 | 直觉 |
|---------|------|------|------|
| **System** | **任一**后继是 Swin | Swin | System 可以选择输出到达 Swin 状态 |
| **System** | **所有**后继是 Ewin | Ewin | System 无论选什么都到 Ewin |
| **Environment** | **所有**后继是 Swin | Swin | Environment 无法避免 Swin |
| **Environment** | **任一**后继是 Ewin | Ewin | Environment 会选到达 Ewin 的输入 |

### 5.2 直觉解释

**System 状态**（System 有主动权）：
- System **想要** Swin，会**选择**有利输出
- 只要**有一个**选择能到 Swin → System 选它 → Swin
- 只有**所有**选择都到 Ewin → System 没法避免 → Ewin

**Environment 状态**（Environment 有主动权，但和 System 对抗）：
- Environment **想要** Ewin，会**选择**对 System 不利的输入
- 只要**有一个**选择能到 Ewin → Environment 选它 → Ewin
- 只有**所有**选择都只能到 Swin → Environment 没法避免 → Swin

### 5.3 ⚠️ 代码中的 BUG

当前代码 `on_the_fly_solver.cpp:442-449` 的 Environment 传播规则是**反的**：

```cpp
// ❌ 错误代码
if (all_ewin && !succs.empty()) {
    classification_[state] = StateClass::Ewin;  // 错误！
} else if (has_swin) {
    classification_[state] = StateClass::Swin;  // 错误！
}

// ✅ 正确逻辑
if (has_ewin) {           // 任一后继是 Ewin
    classification_[state] = StateClass::Ewin;
} else if (all_swin) {    // 所有后继是 Swin
    classification_[state] = StateClass::Swin;
}
```

详见：`docs/BUGS/open.md` [BUG-002]

---

## 6. 循环条件分析

### 6.1 代码

```cpp
while (!worklist_.empty() && !is_initial_classified()) {
    // ...
}
```

### 6.2 条件解析

| 条件 | 含义 | 何时为真 |
|------|------|---------|
| `!worklist_.empty()` | 还有待处理的状态 | worklist 中存在未展开状态 |
| `!is_initial_classified()` | 初始状态未确定胜负 | classification_ 中没有初始状态 |

### 6.3 退出情况

```cpp
// 情况 1: 初始状态被分类 → 可以判定 realizability
if (is_initial_classified()) {
    return (classification_[initial_state_] == StateClass::Swin);
}

// 情况 2: worklist 为空 → 需要检查是否已穷尽所有可达状态
if (worklist_.empty()) {
    // 进入 terminal classification 逻辑
}
```

---

## 7. 循环体五阶段详解

### 7.1 阶段 1: EXPAND（展开状态）

```cpp
// Pop a state to expand
GameState state = worklist_.back();
worklist_.pop_back();

// Skip if already expanded
if (expanded_.count(state)) {
    continue;
}

// Expand state (compute successors)
expand_state(state);
```

**目的**：计算一个状态的所有后继状态

**状态展开规则**：
```cpp
if (state.player == Player::System) {
    // System 回合：枚举所有输出赋值
    for (const auto& out : outputs) {
        succs.push_back(environment_state(state.dfa_state, out));
    }
} else {
    // Environment 回合：枚举所有输入赋值
    for (const auto& in : inputs) {
        automata::Assignment full = state.system_chosen_output.value();
        // 添加输入变量
        for (int v : in) {
            full.insert(v + output_gen_.num_variables());
        }
        automata::TableauState* next_dfa = dfa_.successor(state.dfa_state, full);
        succs.push_back(system_state(next_dfa));
    }
}
```

---

### 7.2 阶段 2: SCC_DECOMPOSE（强连通分解）

```cpp
// Run SCC decomposition on current graph
auto sccs = find_sccs();
```

**目的**：识别游戏图中的强连通分量（循环）

**算法**：Tarjan's SCC Algorithm

**复杂度**：O(V + E)，其中 V = 已展开状态数，E = 边数

**结果**：返回 SCC 列表，SCC 之间有拓扑序

---

### 7.3 阶段 3: SOLVE_SCC（求解 SCC）

**⚠️ 当前代码实现不正确**

正确的做法应该是使用不动点迭代（见第 4 节），但当前代码简化为：

```cpp
// 当前代码（简化版，可能不正确）
auto cls = try_classify_scc(scc);
if (cls) {
    // 标记整个 SCC
    for (const auto& s : scc) {
        classification_[s] = *cls;
    }
}
```

**问题**：
1. "SCC 包含接受状态 → 整个 SCC 是 Swin" 是错误的
2. 应该用不动点迭代，分别判断每个状态

---

### 7.4 阶段 4: PROPAGATE（向后传播）

```cpp
bool initial_done = propagate_classification();
if (initial_done) {
    LOG_DEBUG("OnTheFlyGameSolver: initial state classified!");
    break;
}
```

**目的**：将 SCC 内分类的结果传播到 SCC 外的前驱

**传播方向**：从后继向前驱（backward propagation）

**迭代至不动点**：重复传播直到没有新分类产生

---

### 7.5 阶段 5: ENQUEUE（加入未分类后继）

```cpp
// Add unclassified successors to worklist
for (const auto& pair : successors_) {
    for (const GameState& succ : pair.second) {
        if (!classification_.count(succ) && !expanded_.count(succ)) {
            worklist_.push_back(succ);
        }
    }
}
```

**条件**：状态既未分类也未展开

**目的**：逐步探索整个可达状态空间

---

## 8. 算法流程图

```mermaid
flowchart TD
    A[开始] --> B[初始化 worklist = initial_state]
    B --> C{worklist 不空<br/>且初始状态未分类?}

    C -->|是| D[Pop 状态]
    D --> E{已展开?}
    E -->|是| C
    E -->|否| F[展开状态: 计算后继]

    F --> G[SCC 分解]
    G --> H[按拓扑序遍历每个 SCC]

    H --> I[不动点迭代求解 SCC]
    I --> J[标记 SCC 内所有状态]
    J --> K[向后传播分类]
    K --> L{初始状态已分类?}
    L -->|是| M[跳出循环]
    L -->|否| N[处理下一个 SCC]

    N --> H

    H --> O[所有 SCC 处理完]
    O --> P[添加未分类后继到 worklist]
    P --> C

    C -->|否| Q{worklist 为空?}
    Q -->|是| R[终止状态分类]
    Q -->|否| M

    R --> S[最终传播]
    S --> M

    M --> T[返回 initial_state == Swin]
    T --> U[结束]
```

---

## 9. 复杂度分析

| 阶段 | 单次复杂度 | 总复杂度 |
|------|-----------|---------|
| Expand | O(2^m × 2^n) | O(V × 2^m × 2^n) |
| SCC Decompose | O(V + E) | O(V × (V + E)) |
| Solve SCC (不动点) | O(\|SCC\| × deg) | O(V × E) |
| Propagate | O(V + E) | O(V × (V + E)) |

其中：
- V = 已展开状态数
- E = 边数
- m = 输出变量数
- n = 输入变量数
- deg = 平均度数

**On-the-Fly 优势**：V 可能远小于完整游戏图的状态数

---

## 10. 附录：之前的错误理解

### 10.1 错误 1: SCC 内所有状态相同胜负

> ❌ **错误**："同一 SCC 内的所有状态具有相同的胜负状态"
>
> ✅ **正确**：SCC 内状态需要分别判断，通过不动点迭代

### 10.2 错误 2: 接受状态 SCC 都是 Swin

> ❌ **错误**："SCC 包含接受状态 → 整个 SCC 都是 Swin"
>
> ✅ **正确**：接受状态是 Swin 的种子，需要通过不动点迭代传播

### 10.3 错误 3: 传播规则搞反

> ❌ **错误**：Environment 状态"所有后继 Ewin → Ewin"
>
> ✅ **正确**：Environment 状态"任一后继 Ewin → Ewin"

---

## 11. 真正的 On-The-Fly 优化策略

### 11.1 当前实现的问题

**当前 expand_state 不是真正的 on-the-fly**：

```cpp
// 当前代码：全量展开
auto outputs = output_gen_.all_assignments();  // 枚举所有 2^m 个输出

for (const auto& out : outputs) {  // 全部展开
    succs.push_back(environment_state(state.dfa_state, out));
}
```

| 方面 | 当前实现 | 真正 on-the-fly |
|------|---------|----------------|
| 展开 | 每个状态完全展开所有后继 | 按需展开后继 |
| 内存 | 存储所有后继 | 只存储必要后继 |
| 复杂度 | O(V × 2^m × 2^n) | 与可达状态相关，不与所有赋值相关 |

### 11.2 On-The-Fly 优化策略

#### 策略 1：逐步展开

```
当前：一次性展开所有后继
    sys_move → {env1, env2, env3, ...}  (全部展开)

优化：一次只展开一个
    sys_move → env1 → ...  (只展开这一条路径)
              ↓
           如果 env1 确定了结果，再考虑 env2
```

#### 策略 2：sys-move 的早期终止（Swin）

```
sys 状态选择输出：
    尝试 output1 → env 状态 → ... → 发现是 Swin
    → 直接判定：当前 sys 状态是 Swin
    → 无需尝试其他 output
```

#### 策略 3：sys-move 的剪枝（Ewin）

```
sys 状态选择输出：
    尝试 output1 → env 状态 → ... → 发现是 Ewin
    → 当前 output 不可行，尝试下一个 output
    → 如果所有 output 都导致 Ewin → 当前 sys 状态是 Ewin
```

#### 策略 4：env-move 的剪枝（回退）

```
env 状态选择输入：
    尝试 input1 → sys 状态 → ... → 发现是 Ewin
    → Environment 找到让 System 输的输入！
    → 无需尝试其他 input
    → 直接回退到上一个 sys 状态，尝试下一个 output
```

### 11.3 算法流程（深度优先 + 剪枝）

```
function solve(sys_state):
    for each output in outputs:
        env_state = environment_state(sys_state, output)

        // 检查 env 状态
        result = check_env(env_state)

        if result == Swin:
            // 找到一个能赢的输出
            return Swin

        // result == Ewin，当前 output 不可行
        // 继续尝试下一个 output

    // 所有 output 都导致 Ewin
    return Ewin

function check_env(env_state):
    for each input in inputs:
        next_sys = system_state(env_state, input)

        result = solve(next_sys)

        if result == Ewin:
            // Environment 找到让 System 输的输入！
            // 无需尝试其他 input，直接返回
            return Ewin

    // 所有 input 都导致 Swin
    return Swin
```

### 11.4 剪枝效果

| 场景 | 无剪枝 | 有剪枝 |
|------|-------|-------|
| sys 找到 Swin 输出 | 遍历所有 output | 只遍历到第一个 |
| env 找到 Ewin 输入 | 遍历所有 input | 只遍历到第一个 |
| 最坏情况 | 遍历所有 | 遍历所有 |
| 平均情况 | 大量冗余探索 | 显著减少探索 |

### 11.5 实现要点

1. **深度优先搜索**：一条路径走到底，而不是广度优先展开所有后继
2. **提前返回**：sys-move 找到 Swin → 立即返回
3. **env 剪枝**：env-move 找到 Ewin → 立即返回，不试其他 input
4. **记忆化**：已分类状态缓存，避免重复计算

---

## 12. 相关文件

| 文件 | 说明 |
|------|------|
| `include/synthesis/on_the_fly_solver.hpp` | OnTheFlyGameSolver 类定义 |
| `src/synthesis/on_the_fly_solver.cpp` | 算法实现 |
| `docs/ARCHITECTURE/synthesis.md` | Synthesis 模块架构 |
| `docs/ARCHITECTURE/components.md` | 核心组件设计 |
| `docs/BUGS/open.md` | 已知 Bug 列表 |
