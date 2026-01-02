# Tableau-Based DFA Construction for LTLf

**本文档详细介绍 Tableau 方法将 LTLf 公式转换为 DFA 的原理与实现**

---

## 1. 核心思想

### 1.1 传统方法 vs Tableau 方法

| 方面 | 传统方法 (AALTA/Lydia) | Tableau 方法 |
|------|----------------------|-------------|
| 构建方式 | 一次性构建完整 DFA | 按需构建状态 |
| 状态表示 | BDD 符号化或显式枚举 | 公式集合 (Set of Formulas) |
| 内存占用 | 可能指数级增长 | 仅探索可达状态 |
| 适用场景 | 需要完整 DFA 时 | 游戏求解（可早期终止） |

### 1.2 为什么用公式集合表示状态？

Tableau 方法中，每个 DFA 状态是**原始公式的子公式集合**：

```
示例：φ = p1 ∧ X(p2 ∨ p3)

初始状态 q0 = { p1 ∧ X(p2 ∨ p3) }

展开后：q0' = { p1, X(p2 ∨ p3) }  (And 规则)
```

**优点**：
- 直接反映公式的语义
- 状态转换自然对应语义转换
- 无需预先生成完整自动机

---

## 2. Tableau 构造规则

### 2.1 状态结构

```
TableauState = { ψ₁, ψ₂, ..., ψₙ }

其中每个 ψᵢ 是原始公式 φ 的子公式
```

### 2.2 局部一致性规则 (Tableau 1)

状态 Γ 必须满足以下一致性条件：

| 公式类型 | 一致性要求 |
|---------|-----------|
| `false` | ❌ 不能在 Γ 中 |
| `ψ₁ ∧ ψ₂` | ψ₁ ∈ Γ **且** ψ₂ ∈ Γ |
| `ψ₁ ∨ ψ₂` | ψ₁ ∈ Γ **或** ψ₂ ∈ Γ |
| `ψ₁ U ψ₂` | ψ₂ ∈ Γ **或** (ψ₁ ∈ Γ **且** ψ₁ U ψ₂ ∈ Γ) |
| `ψ₁ R ψ₂` | ψ₂ ∈ Γ |

**矛盾检测**：
```
如果 p ∈ Γ 且 !p ∈ Γ → 状态不一致
```

### 2.3 下一状态计算 (Tableau 2)

给定当前状态 Γ 和赋值 a，下一状态 Γ' 为：

```
Γ' = old(Γ) ∪ next(Γ)
```

其中：

```
old(Γ) = { ψ ∈ Γ | ψ 不是 Next/Until/Release }
next(Γ) = { ψ' | ○ψ ∈ Γ 且 ψ' 是 ψ 的子公式 }
         ∪ { ψ₂ | ψ₁ R ψ₂ ∈ Γ }
```

**具体规则**：

| 当前 Γ 中的公式 | 添加到 Γ' |
|----------------|----------|
| 布尔/文字 | 如果对 a 为真则保留 |
| `ψ₁ ∧ ψ₂` | ψ₁, ψ₂ (已在 old 中) |
| `ψ₁ ∨ ψ₂` | 选中为真的分支 (已在 old 中) |
| `○ψ` | ψ (展开到 next) |
| `ψ₁ U ψ₂` | 如果 ψ₂ 对 a 为真则移除，否则保留 |
| `ψ₁ R ψ₂` | ψ₂ (继续), ψ₁ R ψ₂ (保留) |

---

## 3. 示例推导

### 示例 1: 简单公式

```
φ = F p = true U p
```

```
Step 0: q0 = { true U p }
        ↓
        [应用 Until 规则]
        ↓
Step 1: q0 = { true, true U p }  (一致性检查通过)
        ↓
        [赋值: p = false]
        ↓
Step 2: q1 = { true U p }  (true 移除, Until 保留因为 p 不满足)
        ↓
        [赋值: p = true]
        ↓
Step 3: q2 = { }  (Until 移除因为 p 满足, true 移除)
```

**状态转换图**：
```
        {true U p}
       /          \
   p=false        p=true
     /               \
 {true U p}          {}
    |
   (循环，直到 p=true)
```

### 示例 2: Until 公式

```
φ = p1 U p2
```

```
q0 = { p1 U p2 }

应用 Until 一致性规则：
- 需要 p2 ∈ Γ 或 (p1 ∈ Γ 且 p1 U p2 ∈ Γ)

选择分支：q0 = { p1, p1 U p2 }  (假设 p2 当前不满足)
```

### 示例 3: Release 公式

```
φ = p1 R p2  (等价于 G p2 ∨ (p2 U (p1 ∧ p2)))
```

```
q0 = { p1 R p2 }

应用 Release 一致性规则：
- 需要 p2 ∈ Γ

下一状态计算：
- old: p2 (保留)
- next: p2 (Release 继续), p1 R p2 (保留 Release)

q' = { p2, p1 R p2 }
```

### 示例 4: 复杂公式

```
φ = G(p → F q)
  = !(p U !q) R false
```

```
q0 = { !(p U !q) R false }

NNF 转换后：
q0 = { (p U !q) → false }
   = { !(p U !q) ∨ false }
   = { !(p U !q) }

（这里需要更复杂的推导...）
```

---

## 4. 实现细节

### 4.1 Hash Consing (状态去重)

```cpp
// 相同的公式集合 → 同一个状态对象
TableauStatePool pool;

auto s1 = pool.get_or_create({p1, p2});
auto s2 = pool.get_or_create({p2, p1});  // 相同内容，返回同一对象

assert(s1 == s2);  // true
```

### 4.2 转移缓存

```cpp
// 缓存 (state, assignment) -> next_state
// 避免重复计算相同的转移
using CacheKey = std::pair<TableauState*, Assignment>;
std::unordered_map<CacheKey, TableauState*> cache_;
```

### 4.3 文字求值

```cpp
// 正文字面 p: 保留当 p ∈ assignment
// 负文字面 !p: 保留当 p ∉ assignment
bool literal_value(Formula* f, const Assignment& assignment) {
    if (f->op() == OpType::Literal) {
        return assignment.count(f->var_id()) > 0;
    }
    if (f->op() == OpType::Not) {
        return assignment.count(f->left()->var_id()) == 0;
    }
    return true;
}
```

---

## 5. 接受状态判定

### 5.1 接受条件

Tableau 状态 Γ 是**接受状态**当且仅当：

1. `false ∉ Γ`
2. Γ 局部一致
3. **对所有 `ψ₁ U ψ₂ ∈ Γ`，必须有 `ψ₂ ∈ Γ`**（没有未完成的 Until）

### 5.2 为什么需要第3条？

在 LTLf 中，`ψ₁ U ψ₂` 的语义是：
- ψ₁ 持续为真，**直到 ψ₂ 为真**
- **ψ₂ 必须最终为真**（有限轨迹必须看到 ψ₂）

如果轨迹结束时状态中还有 `ψ₁ U ψ₂` 但 ψ₂ ∉ Γ，说明这个 Until 还在等待中，轨迹不能在此成功结束。

### 5.3 直观理解

```
接受状态 = 当前所有约束都能同时满足
        + 没有矛盾 (如 p 和 !p 同时存在)
        + 布尔连接词约束满足 (∧ 的两边都在，∨ 至少一边在)
        + 没有未完成的 Until (所有 Until 的右侧都已满足)
```

---

## 6. 代码结构

### 6.1 类层次

```
TableauState (状态)
    ├── formulas_: 公式集合
    ├── hash_: 缓存的哈希值
    ├── is_locally_consistent(): 一致性检查
    ├── is_accepting(): 接受状态检查
    └── next(): 计算下一状态

TableauStatePool (状态池，Hash Consing)
    ├── states_: 所有唯一状态
    ├── storage_: 内存所有权
    └── get_or_create(): 获取或创建状态

OnTheFlyDFA (按需 DFA)
    ├── state_pool_: 状态池
    ├── initial_state_: 初始状态
    ├── transition_cache_: 转移缓存
    └── successor(): 获取/计算后继状态
```

### 6.2 关键算法

```cpp
// 局部一致性检查
bool is_locally_consistent() {
    if (contains(false)) return false;
    for (And in formulas) {
        if (!contains(left) || !contains(right)) return false;
    }
    for (Or in formulas) {
        if (!contains(left) && !contains(right)) return false;
    }
    for (Until in formulas) {
        if (!contains(right) && !contains(left)) return false;
    }
    // 检查矛盾 p 和 !p
    return true;
}

// 下一状态计算
TableauState* next(Assignment a) {
    FormulaSet next_formulas;

    // old(Γ): 保留非时序公式
    for (f in formulas) {
        if (!is_temporal(f) && literal_true(f, a)) {
            next_formulas.insert(f);
        }
    }

    // next(Γ): 展开 Next 和 Release
    for (f in formulas) {
        if (f == Next(ψ)) {
            next_formulas.insert(ψ);
        } else if (f == Release(ψ₁, ψ₂)) {
            next_formulas.insert(ψ₂);  // Release 继续
            next_formulas.insert(f);    // 保留 Release
        } else if (f == Until(ψ₁, ψ₂)) {
            if (!literal_true(ψ₂, a)) {
                next_formulas.insert(f);  // Until 继续
            }
        }
    }

    return pool.get_or_create(next_formulas);
}
```

---

## 7. 与游戏求解的集成

### 7.1 游戏状态

```
GameState = (DFA状态, 当前玩家, 输出赋值)

- System 轮: 选择输出赋值 → Environment 轮
- Environment 轮: 选择输入赋值 → 下一 DFA 状态
```

### 7.2 SCC 分类

```
在 SCC 中：
- 如果包含接受 DFA 状态 → Swin (系统必胜)
- 否则 → Ewin (环境必胜)
```

---

## 8. 复杂度分析

### 8.1 理论复杂度

| 操作 | 复杂度 |
|------|--------|
| 创建初始状态 | O(1) |
| 局部一致性检查 | O(|Γ|²) |
| 下一状态计算 | O(|Γ|) |
| 状态比较 | O(1) (使用哈希) |

### 8.2 空间复杂度

- 最坏情况: O(2^|φ|) 个状态
- 平均情况: 仅探索可达部分
- On-the-fly: 可早期终止

---

## 9. 参考实现

**代码位置**:
- `include/automata/tableau.hpp` - 接口定义
- `src/automata/tableau.cpp` - 实现细节
- `tests/dfa_test.cpp` - 单元测试

**相关论文**:
- arXiv:2408.07324 - Section 3: Tableau Construction
- "Tableau Methods for LTL" (传统 tableau 方法)
- "On-the-Fly Tableau for LTLf" (有限轨迹变体)

---

## 10. 常见问题

### Q1: 为什么需要 NNF 转换？

A: Tableau 规则假设公式在否定范式下，这样不需要处理 `¬¬ψ`、`¬(ψ₁ ∧ ψ₂)` 等情况。

### Q2: Until 和 Release 的区别？

A:
- `ψ₁ U ψ₂`: ψ₁ 持续直到 ψ₂ 为真
- `ψ₁ R ψ₂`: ψ₂ 保持，直到 ψ₁ 为真 (G ψ₂ ∨ (ψ₂ U (ψ₁ ∧ ψ₂)))

### Q3: 为什么 Release 要保留自身？

A: Release 表示"持续释放"，即 ψ₂ 必须持续为真，直到 ψ₁ 也为真。需要保留 Release 公式以持续检查 ψ₂。

### Q4: 状态何时终止？

A:
- 达到接受状态 (游戏可结束)
- 进入循环 (需 SCC 分析)
- 不一致状态 (系统必败)
