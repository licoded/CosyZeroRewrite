# XNF (neXt Normal Form) 详细说明

## 论文信息

- **标题**: Efficient LTLf Synthesis using Next Normal Form
- **作者**: [参见论文]
- **年份**: 2023
- **链接**: https://arxiv.org/pdf/2302.13825
- **本地路径**: `docs/papers/xnf_paper.pdf`

## 记号说明

| 论文记号 | 含义 | 对应 LTL | 备注 |
|---------|------|----------|------|
| ◦ | Next | X | 下一状态必须满足 |
| • | Weak Next | !X 或 W | 下一状态或结束 |
| ♢ | Eventually | F | 某时刻为真 |
| □ | Globally | G | 总是为真 |
| pa(φ) | 当前状态的原子公式 | - | 只包含 literals、X-formulas、Weak Next |

## XNF 定义

**LTLf 公式 φ 在 XNF 中**，如果 **pa(φ)** 只包括：
1. **Literals**: `p1`, `!p1`
2. **◦-formulas**: `X(φ)` (Next formulas)
3. **•-formulas**: `!X(φ)` (Weak Next formulas)

## XNF 转换函数 xnf(φ)

对于 **NNF 公式** φ，xnf(φ) 定义为：

### 基础情况
```
xnf(φ) = φ  如果 φ 是:
  - literal (p1, !p1)
  - □false (G(false))
  - ♢true (F(true))
  - ◦-formula (X(φ))
  - •-formula (!X(φ))
```

### And/Or (保持结构)
```
xnf(φ1 ∧ φ2) = xnf(φ1) ∧ xnf(φ2)
xnf(φ1 ∨ φ2) = xnf(φ1) ∨ xnf(φ2)
```

### Until (转换成 DNF)
```
xnf(φ1 U φ2) = (xnf(φ2) ∧ ♢true) ∨ (xnf(φ1) ∧ ◦(φ1 U φ2))
```

**解释**：
- **分支 1**: `xnf(φ2) ∧ ♢true` - 右侧现在为真，且存在某个时刻
- **分支 2**: `xnf(φ1) ∧ ◦(φ1 U φ2)` - 左侧现在为真，继续等待 Until

### Release (转换成 CNF)
```
xnf(φ1 R φ2) = (xnf(φ2) ∨ □false) ∧ (xnf(φ1) ∨ •(φ1 R φ2))
```

**解释**：
- **合取项 1**: `xnf(φ2) ∨ □false` - 右侧必须为真（除非永远不会为真）
- **合取项 2**: `xnf(φ1) ∨ •(φ1 R φ2)` - 左侧为真或继续 Release

## 转换示例

### 示例 1: 简单公式
```
p1          → {p1}
!p1         → {!p1}
X(p1)       → {X(p1)}
X(!p1)      → {X(!p1)}
```

### 示例 2: Until
```
p1 U p2
→ (p2 ∧ F(true)) ∨ (p1 ∧ X(p1 U p2))
```

初始状态有两个可能的分支：
1. `p2` 为真（且需要存在某个时刻）
2. `p1` 为真，然后继续 `X(p1 U p2)`

### 示例 3: Release
```
p1 R p2
→ (p2 ∨ G(false)) ∧ (p1 ∨ !X(p1 R p2))
```

初始状态需要同时满足：
1. `p2` 为真（或永远不会为真）
2. `p1` 为真或继续 `!X(p1 R p2)`

### 示例 4: G p1 (false R p1)
```
G p1 = false R p1
→ (p1 ∨ G(false)) ∧ (false ∨ !X(false R p1))
→ p1 ∧ !X(false R p1)
```

所以 `G p1` 的 XNF 就是 `p1` 在当前状态为真，然后继续 `!X(false R p1)`。

## 实现要点

### 1. Until 转换 (关键)

**当前实现的问题**：
- 只是展开子公式：`{p1, p2, (p1 U p2)}`
- 没有转换成 Or 形式

**正确实现**：
- `p1 U p2` 应该变成 `(p2 ∧ F(true)) ∨ (p1 ∧ X(p1 U p2))`
- 这意味着初始状态有两个"分支"或"选择"

### 2. Release 转换 (关键)

**当前实现的问题**：
- 只是展开子公式：`{p1, p2, (p1 R p2)}`

**正确实现**：
- `p1 R p2` 应该变成 `(p2 ∨ G(false)) ∧ (p1 ∨ !X(p1 R p2))`
- 这是 And 形式，两个条件都要满足

### 3. F(p1) 和 G(p1)

```
F(p1) = true U p1
→ (p1 ∧ F(true)) ∨ (true ∧ X(true U p1))
→ p1 ∨ X(true U p1)
→ p1 ∨ X(F(p1))
```

```
G(p1) = false R p1
→ (p1 ∨ G(false)) ∧ (false ∨ !X(false R p1))
→ p1 ∧ !X(false R p1)
→ p1 ∧ !X(G(p1))
```

## 定理

**定理 1 (Li et al. 2019)**: 每个 LTLf 公式 φ 在 NNF 下，可以在线性时间内转换为等价的 XNF 公式 xnf(φ)。

## 实现计划

1. **添加 XNF 转换函数**：在 `formula/xnf.cpp` 中实现 `xnf()` 函数
2. **处理 Until/Release**：按照上述规则转换
3. **简化 F/G**：F(p1) → p1 ∨ X(F(p1))，G(p1) → p1 ∧ !X(G(p1))
4. **修改 initial_state**：使用 XNF 转换后的公式构建初始状态
