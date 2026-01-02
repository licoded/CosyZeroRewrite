# XNF (neXt Normal Form) 转换

## 定义来源

论文: [Efficient LTLf Synthesis using Next Normal Form](https://arxiv.org/pdf/2302.13825)

## 记号

| 记号 | 含义 | 对应运算符 |
|------|------|------------|
| ◦ | Next | X |
| • | Weak Next | !X (W-Next) |
| ♢ | Eventually | F (某时刻为真) |
| □ | Globally | G (总是为真) |

## XNF 定义

LTLf 公式 φ 在 XNF 中，如果 **pa(φ)** 只包括：
- Literals (p1, !p1)
- ◦-formulas (X(p1))
- •-formulas (!X(p1), 即 Weak Next)

## XNF 转换函数 xnf(φ)

对于 NNF 公式 φ：

```
xnf(φ) = φ                          如果 φ 是 literal, □false, ♢true, ◦-, •-formula

xnf(φ1 ∧ φ2) = xnf(φ1) ∧ xnf(φ2)

xnf(φ1 ∨ φ2) = xnf(φ1) ∨ xnf(φ2)

xnf(φ1 U φ2) = (xnf(φ2) ∧ ♢true) ∨ (xnf(φ1) ∧ ◦(φ1 U φ2))

xnf(φ1 R φ2) = (xnf(φ2) ∨ □false) ∧ (xnf(φ1) ∨ •(φ1 R φ2))
```

## 关键点

1. **XNF 一定是 NNF 格式**
2. **pa(φ) 是当前状态需要满足的公式集合**
3. **Until/Release 的转换**：
   - Until 被分解为"现在满足右侧"或"继续等待"
   - Release 被分解为"右侧必须保持"的逻辑

## 示例

### 简单公式
- `p1` → `{p1}` ✓
- `!p1` → `{!p1}` ✓
- `X(p1)` → `{X(p1)}` ✓
- `X(!p1)` → `{X(!p1)}` ✓

### Until 公式
- `p1 U p2` → `(p2 ∧ F(true)) ∨ (p1 ∧ X(p1 U p2))`
  - 要么 p2 现在为真（且存在某个时刻）
  - 要么 p1 现在为真，然后继续等待 p1 U p2

### Release 公式
- `p1 R p2` → `(p2 ∨ G(false)) ∧ (p1 ∨ !X(p1 R p2))`
  - p2 必须保持为真（除非永远不会为真）
  - 要么 p1 为真，要么继续 !X(p1 R p2)

## 与当前实现的关系

**问题**：当前实现没有正确实现 Until/Release 的 XNF 转换。

**当前实现**：直接展开 Until/Release 的子公式
- `p1 U p2` → `{p1, p2, (p1 U p2)}`

**正确 XNF**：应该转换为 DNF 形式
- `p1 U p2` → `(p2 ∧ F(true)) ∨ (p1 ∧ X(p1 U p2))`

这意味着在初始状态中，`p1 U p2` 不是一个"分量"，而应该被转换成两个可能的分支。
