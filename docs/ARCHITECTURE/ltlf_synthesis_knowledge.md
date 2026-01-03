# LTLf Synthesis Core Knowledge

## XNF (neXt Normal Form) 转换

### 核心原理
通过 XNF 转换，所有 LTLf 公式都转换为统一模式：
- **当前步需要满足的命题**（原子公式、布尔组合）
- **Next 转移**（下一步需要满足的公式）

### XNF 转换规则

| 算子 | XNF 转换 | 说明 |
|------|----------|------|
| `X(φ)` | `φ` 在下一步 | 直接转移到下一步 |
| `F(φ)` | `φ \| X(F(φ))` | 当前满足或下一步满足 |
| `G(φ)` | `φ & X(G(φ))` | 当前必须满足且下一步也要满足 |
| `φ U ψ` | `ψ \| (φ & X(φ U ψ))` | 当前满足ψ，或保持φ并继续 |
| `φ R ψ` | `ψ & (φ \| X(φ R ψ))` | ψ必须当前满足，φ或满足或继续 |
| `φ W ψ` | `(φ & X(φ W ψ)) \| ψ` | 类Until但ψ可以不出现 |
| `φ M ψ` | `ψ & (φ & X(φ M ψ))` | 类Release |

### 关键洞察
**不需要特殊情况处理**：XNF 转换已经将所有 temporal 算子分解为：
1. 当前步的命题要求（edge 上的 AP）
2. 下一步的状态转移（next 状态）

任何特殊的 formula 模式都应该被 XNF 正确处理，不需要额外的 ad-hoc 检查。

## Empty Trace Accepting

### 定义
一个公式在**空串**（length 0）上是否可以被满足。

### 规则

| 算子 | Empty Trace Accepting | 原因 |
|------|----------------------|------|
| `X(φ)` | ❌ NO | 需要至少一个时间点 |
| `φ U ψ` | ❌ NO | Until 需要ψ在某个未来时间点为真 |
| `F(φ)` | ❌ NO | Eventually 需要未来时间点 |
| `φ R ψ` | ✅ YES | Release 的 ψ 可以立即满足 |
| `G(φ)` | ✅ YES | Globally 在空串上 vacuously true |
| `φ W ψ` | ❌ NO | Weak Until 仍需ψ或无限φ |
| `φ M ψ` | ✅ YES | Strong Release 类Release |
| **命题** `p` | ❌ NO | 空串无命题可满足 |
| **布尔组合** | ❌ NO | 取决于子公式 |

### 在 Synthesis 中的应用

**System Move 前检查**：
- 在 system 选择输出前，检查当前状态是否可以终止（empty trace accepting）
- 如果状态包含 U、X、F（不能空串满足），系统不能在此终止
- 如果状态只包含 R、G、WX（可以空串满足），系统可以选择在此终止

**与 Accepting State 的关系**：
- Empty trace accepting ≠ accepting state in tableau
- Tableau accepting: 无矛盾、局部一致
- Empty trace accepting: 公式可以在长度为0的 trace 上满足

## 当前问题诊断

### 错误的调试方向
1. ❌ 添加特殊情况模式检测（如 `A & (B | !A)`）
2. ❌ 在 `requires_input_true` 中添加复杂逻辑
3. ❌ 为每个 corner case 添加专门处理

### 正确的调试方向
1. ✅ 检查 XNF 转换是否正确
2. ✅ 检查 empty trace accepting 规则是否正确应用
3. ✅ 检查 tableau state 的 accepting 判断
4. ✅ 检查 game solver 的胜负判定

### 需要检查的代码位置
1. `TableauState::initial()` - XNF 转换
2. `OnTheFlyDFA::is_accepting()` - accepting 状态判断
3. `OnTheFlyGameSolver::is_realizable()` - 胜负判定
4. Empty trace accepting 检查逻辑

## 参考实现对比

### Cosy 的处理方式
- 使用完整的 automata construction
- 明确的 empty trace 检查
- 标准 tableau 算法

### 我们需要做的
- 确保与 Cosy 语义一致
- 简化逻辑，依赖 XNF 而非特殊处理
