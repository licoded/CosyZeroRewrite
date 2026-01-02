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

**论文原式**:
```
xnf(φ1 U φ2) = (xnf(φ2) ∧ ♢true) ∨ (xnf(φ1) ∧ ◦(φ1 U φ2))
```

**我们的实现**（不使用 F(true)）:
```
xnf(φ1 U φ2) = xnf(φ2) ∨ (xnf(φ1) ∧ X(φ1 U φ2))
                         └──────────────┘
                         X 是 strong next，隐含 !End
```

**关键点**：
- Until 公式**不能被空串接受**（必须继续直到 φ2 为真）
- X 是 strong next，隐含 `!End` 约束
- 分支 1: `φ2` 现在为真（ Until 满足）
- 分支 2: `φ1` 现在为真，然后继续 `X(φ1 U φ2)`

### Release (转换成 CNF)

**论文原式**:
```
xnf(φ1 R φ2) = (xnf(φ2) ∨ □false) ∧ (xnf(φ1) ∨ •(φ1 R φ2))
```

**我们的实现**（不使用 G(false)）:
```
xnf(φ1 R φ2) = (xnf(φ2) ∨ End) ∧ (xnf(φ1) ∨ WX(φ1 R φ2))
                                  └──────────────┘
                                  WX 是 weak next，允许 End
```

**关键点**：
- Release 公式**可以被空串接受**（如果 φ2 已经满足）
- WX (Weak Next) 允许轨迹结束
- 合取项 1: `φ2` 为真 **或** 轨迹结束（空串满足 Release）
- 合取项 2: `φ1` 为真 **或** 继续 `WX(φ1 R φ2)`

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
→ p2 ∨ (p1 ∧ X(p1 U p2))
```

初始状态有两个可能的分支：
1. `p2` 为真（Until 满足，可以结束）
2. `p1` 为真，然后继续 `X(p1 U p2)`（X 隐含 !End）

### 示例 3: Release
```
p1 R p2
→ (p2 ∨ End) ∧ (p1 ∨ WX(p1 R p2))
```

初始状态需要同时满足：
1. `p2` 为真 **或** 轨迹可以结束
2. `p1` 为真 **或** 继续 `WX(p1 R p2)`

### 示例 4: G p1 (false R p1)
```
G p1 = false R p1
→ (p1 ∨ End) ∧ (false ∨ WX(false R p1))
→ p1 ∧ WX(false R p1)
```

所以 `G p1` 的 XNF 就是 `p1` 在当前状态为真，然后继续 `WX(false R p1)`。
- 如果选择 `End`，需要 `p1` 为真（但 G p1 要求每步都为真，不能只一步）
- 因此必须继续 `WX(false R p1)`，即下一步仍需满足 `G p1`

## 实现要点

### 1. Until 转换 (关键)

**当前实现的问题**：
- 只是展开子公式：`{p1, p2, (p1 U p2)}`
- 没有转换成 Or 形式

**正确实现**：
- `p1 U p2` → `p2 ∨ (p1 ∧ X(p1 U p2))`
- X 隐含 `!End` 约束，意味着必须继续
- 初始状态有两个"分支"或"选择"

### 2. Release 转换 (关键)

**当前实现的问题**：
- 只是展开子公式：`{p1, p2, (p1 R p2)}`

**正确实现**：
- `p1 R p2` → `(p2 ∨ End) ∧ (p1 ∨ WX(p1 R p2))`
- WX (Weak Next) 允许轨迹结束
- 这是 And 形式，两个条件都要满足

### 3. F(p1) 和 G(p1)

```
F(p1) = true U p1
→ p1 ∨ (true ∧ X(true U p1))
→ p1 ∨ X(F(p1))
```

```
G(p1) = false R p1
→ (p1 ∨ End) ∧ (false ∨ WX(false R p1))
→ p1 ∧ WX(G(p1))
```

### 4. End 标记的使用

- **X (Strong Next)**: 隐含 `!End`，轨迹不能在此结束
- **WX (Weak Next)**: 允许 `End`，轨迹可以在此结束
- 判断空串是否接受：检查公式能否被空串满足（Release 类公式通常可以）

## 定理

**定理 1 (Li et al. 2019)**: 每个 LTLf 公式 φ 在 NNF 下，可以在线性时间内转换为等价的 XNF 公式 xnf(φ)。

## 实现计划

1. **添加 XNF 转换函数**：在 `formula/xnf.cpp` 中实现 `xnf()` 函数
2. **处理 Until/Release**：按照上述规则转换
3. **简化 F/G**：F(p1) → p1 ∨ X(F(p1))，G(p1) → p1 ∧ !X(G(p1))
4. **修改 initial_state**：使用 XNF 转换后的公式构建初始状态
