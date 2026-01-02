# LTLf Synthesis 架构详解

> CosyZeroRewrite 项目中 Synthesis 模块的完整架构

**最后更新**: 2026-01-02

---

## 目录

1. [核心概念](#核心概念)
2. [Synthesis 架构](#synthesis-架构)
3. [两个实现版本对比](#两个实现版本对比)
4. [算法流程图](#算法流程图)
5. [输入输出分离](#输入输出分离)

---

## 核心概念

### 什么是 LTLf Synthesis？

**问题**: 给定一个 LTLf 公式 φ，系统是否总能选择输出值使得 φ 成立（无论环境如何选择输入）？

**双人博弈**:
- **System (系统)**: 控制输出变量
- **Environment (环境)**: 控制输入变量
- **目标**: 系统需要找到**对抗所有环境选择**的获胜策略

```
           ┌─────────────────┐
           │   Synthesis     │
           │   Problem       │
           └────────┬────────┘
                    │
        ┌───────────┴───────────┐
        │                       │
   ┌────▼────┐            ┌─────▼────┐
   │ System  │            │Environment│
   │ Outputs │            │  Inputs  │
   └────┬────┘            └─────┬────┘
        │                       │
        │   系统选择输出  →  环境选择输入  →  循环...
        │
    谁有必胜策略？
```

### 语义层次

| 层次 | 说明 | 示例 |
|------|------|------|
| LTLf Formula | 用户编写的时序逻辑公式 | `G (req → F ack)` |
| NNF | 否定范式（否定只出现在原子命题前） | `G (!req ∨ F ack)` |
| Tableau DFA | 从公式构造的自动机 | 状态 + 转移 |
| Game Graph | 扩展了玩家信息的游戏图 | (DFA状态, 玩家, 赋值) |

---

## Synthesis 架构

### 整体架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                        LTLf Synthesis System                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  用户输入                                                            │
│  ┌────────────────┐    ┌────────────────┐                         │
│  │ Formula:       │    │ Partition:     │                         │
│  │ "G(req→F ack)" │    │ .outputs: ack  │                         │
│  │                │    │ .inputs:  req  │                         │
│  └────────┬───────┘    └────────┬───────┘                         │
│           │                     │                                  │
│           └──────────┬──────────┘                                  │
│                      ▼                                             │
│           ┌──────────────────────┐                                  │
│           │  FormulaParser       │                                  │
│           │  (解析 + 变量声明)     │                                  │
│           └──────────┬───────────┘                                  │
│                      ▼                                             │
│           ┌──────────────────────┐                                  │
│           │  FormulaPool         │                                  │
│           │  (Hash Consing 存储)  │                                  │
│           └──────────┬───────────┘                                  │
│                      │                                             │
│      ┌───────────────┼───────────────┐                              │
│      ▼               ▼               ▼                              │
│ ┌──────────┐  ┌──────────┐  ┌──────────────────┐                      │
│ │ DFABuilder│  │OnTheFly  │  │  TableauState    │                      │
│ │          │  │GameSolver│  │  + OnTheFlyDFA   │                      │
│ └─────┬────┘  └─────┬────┘  └──────────────────┘                      │
│       │             │                                              │
│       │             │  ←【当前主要使用】                              │
│       ▼             ▼                                              │
│  ┌──────────────────────────────────────┐                            │
│  │       SCC Decomposition (Tarjan)    │                            │
│  └───────────────┬──────────────────────┘                            │
│                  ▼                                                   │
│  ┌──────────────────────────────────────┐                            │
│  │    State Classification (Swin/Ewin) │                            │
│  └───────────────┬──────────────────────┘                            │
│                  ▼                                                   │
│  ┌──────────────────────────────────────┐                            │
│  │     Backward Propagation            │                            │
│  └───────────────┬──────────────────────┘                            │
│                  ▼                                                   │
│           ┌─────────────┐                                           │
│           │ Realizable? │                                           │
│           │   YES / NO   │                                           │
│           └─────────────┘                                           │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 两个实现版本对比

### 版本概览

| 特性 | DFABuilder 版本 | OnTheFlyGameSolver 版本 |
|------|----------------|------------------------|
| **文件位置** | `src/automata/dfa.cpp` | `src/synthesis/on_the_fly_solver.cpp` |
| **状态表示** | `StateFormulaSet` | `GameState = (TableauState*, Player, Assignment)` |
| **DFA构造** | 完整构造 | 按需构造 |
| **玩家区分** | ❌ 无 | ✅ 明确区分 System/Environment |
| **内存使用** | 较高（全量构造） | 较低（懒加载） |
| **当前状态** | 基础实现完成 | 主要实现，持续优化中 |
| **调用入口** | `GameSolver::is_realizable()` | `is_realizable_on_the_fly()` ← Cosy2主入口 |

### DFABuilder 版本 (静态 DFA 构造)

**核心思想**: 先构造完整 DFA，再进行游戏求解

```
输入公式 φ
    │
    ▼
┌──────────────────────────┐
│ 1. 转换为 XNF            │
│    (Next 范式)            │
└──────────┬───────────────┘
           │
           ▼
┌──────────────────────────┐
│ 2. 提取原子命题          │
│    primitives = {p, Xp}   │
└──────────┬───────────────┘
           │
           ▼
┌──────────────────────────┐
│ 3. 生成所有 MCS          │
│    (Maximal Consistent   │
│     Sets) 作为状态        │
└──────────┬───────────────┘
           │
           ▼
┌──────────────────────────┐
│ 4. 构造转移              │
│    基于 X-公式            │
└──────────┬───────────────┘
           │
           ▼
┌──────────────────────────┐
│ 5. 标记接受状态          │
│    is_accepting_state()  │
└──────────┬───────────────┘
           │
           ▼
      完整 DFA
```

**问题**: 没有区分输入/输出，不适合 synthesis

### OnTheFlyGameSolver 版本 (动态游戏求解)

**核心思想**: 边构造游戏图边求解，明确区分玩家

```
输入公式 φ + FormulaPool (带变量分区)
    │
    ▼
┌──────────────────────────────────┐
│ 1. 创建 OnTheFlyDFA              │
│    initial_state = {φ及其子公式}  │
└──────────┬───────────────────────┘
           │
           ▼
┌──────────────────────────────────┐
│ 2. 初始化游戏状态                │
│    (DFA初始态, System回合)         │
└──────────┬───────────────────────┘
           │
           ▼
┌──────────────────────────────────┐
│ 3. 循环: 展开状态               │
│    a) 系统回合: 生成所有输出赋值  │
│    b) 环境回合: 生成所有输入赋值  │
│    c) 计算 next DFA state        │
└──────────┬───────────────────────┘
           │
           ▼
┌──────────────────────────────────┐
│ 4. SCC 分解 (Tarjan)             │
│    检测循环                       │
└──────────┬───────────────────────┘
           │
           ▼
┌──────────────────────────────────┐
│ 5. SCC 分类                      │
│    • SCC包含接受状态 → Swin      │
│    • SCC所有后继是Swin → Swin     │
│    • 否则 → Ewin 或继续          │
└──────────┬───────────────────────┘
           │
           ▼
┌──────────────────────────────────┐
│ 6. 向后传播分类                  │
│    System: 任意后继Swin → Swin  │
│    Environment: 所有后继Swin → Swin│
└──────────┬───────────────────────┘
           │
           ▼
    初始状态分类 = 答案
```

---

## 算法流程图

### 详细流程图 (Mermaid 可视化)

```mermaid
flowchart TD
    Start([LTLf Formula输入]) --> Parse[Parser解析公式]
    Parse --> Pool[FormulaPool变量分区]
    Pool --> Init[创建初始DFA状态]

    Init --> GameStart[初始化游戏状态]
    GameStart --> Expand{状态是否已展开}

    Expand -->|需要展开| SysTurn[System回合选择输出]
    Expand -->|已展开| CheckDone{初始状态分类}

    SysTurn --> EnvTurn[Environment回合选择输入]
    EnvTurn --> Next[计算next DFA状态]

    Next --> AddSucc[添加到后继列表]
    AddSucc --> SCC[Tarjan SCC分解]

    SCC --> Classify{SCC分类结果}

    Classify -->|Swin| Propagate[向后传播]
    Classify -->|Ewin| Propagate
    Classify -->|未定| MoreWork[需要展开更多状态]

    Propagate --> CheckDone
    MoreWork --> Expand

    CheckDone -->|已分类| End([返回Realizable结果])
    CheckDone -->|未分类| MoreWork
```

### 数据结构关系图

```
┌─────────────────────────────────────────────────────────────┐
│                     FormulaPool                            │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ variables_: outputs + inputs                          │   │
│  │   num_outputs_ = 2  (p0, p1)                        │   │
│  │   num_inputs_  = 1  (p2)                            │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ pool_: Hash Set of Formula*                        │   │
│  │   ¬(X(G(p5))) = !X(G(p5))                          │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ create/parse
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    TableauState                             │
│  formulas_: FormulaSet = {f1, f2, ...}                    │
│    e.g., {¬(X(G(p5))), X(p6), true, ...}                  │
│                                                             │
│  is_accepting():                                           │
│    - 无 false                                               │
│    - 局部一致                                               │
│    - Until右式满足                                          │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ DFA状态
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                      GameState                               │
│  dfa_state: TableauState*                                  │
│  player: System | Environment                              │
│  current_output: Assignment (仅Environment回合有值)           │
│                                                             │
│  successors: vector<GameState>                             │
│    System回合: 后继是所有可能的 (环境选择输入后的DFA状态)     │
│    Environment回合: 后继是所有可能的 (系统选择输出后的环境状态) │
└─────────────────────────────────────────────────────────────┘
```

---

## 输入输出分离

### 变量分区 (Variable Partitioning)

```
所有变量 = outputs ∪ inputs

.outputs: p0, p1  ← System 控制
.inputs:  p2, p3  ← Environment 控制
```

### 编号规则

```cpp
// FormulaPool 中的编号
outputs:  id = 0, 1, ..., num_outputs-1
inputs:   id = num_outputs, ..., num_outputs+num_inputs-1

// 判断变量类型
if (var_id < num_outputs) {
    // 这是输出，系统可以控制
} else {
    // 这是输入，环境控制
}
```

### 游戏展开中的赋值

```
System 回合:
  outputs_gen 生成所有输出赋值 (2^num_outputs 种)
  每种赋值 → Environment状态

Environment 回合:
  inputs_gen 生成所有输入赋值 (2^num_inputs 种)
  合并 current_output + input → full_assignment
  full_assignment → next DFA state
```

---

## 关键类关系

```cpp
// 核心关系
FormulaPool
    ├── Formula* (存储所有公式)
    └── num_outputs_, num_inputs_ (变量分区信息)

TableauState
    ├── FormulaSet formulas_ (状态中的公式集合)
    └── is_accepting() (接受状态判定)

OnTheFlyDFA
    ├── TableauStatePool state_pool_ (状态池)
    ├── FormulaPool& pool_ (公式池引用)
    ├── int num_outputs_ (输出数量，用于synthesis检查)
    └── is_accepting(TableauState*) → bool (synthesis语义的接受判定)

OnTheFlyGameSolver
    ├── OnTheFlyDFA dfa_ (DFA)
    ├── AssignmentGenerator output_gen_ (输出生成器)
    ├── AssignmentGenerator input_gen_ (输入生成器)
    ├── classification_ (状态分类: Swin/Ewin/Unknown)
    └── successors_ (后继关系)
```

---

## 下一步

- [算法复杂度分析](./algorithms.md)
- [组件详细设计](./components.md)
- [实现路线图](./roadmap.md)
