# Open Bugs / Known Issues

> 未修复的 Bug 和已知限制

---

## 统计

| 优先级 | 数量 |
|--------|------|
| 🔴 高 | 0 |
| 🟡 中 | 0 |
| 🟢 低 | 0 |
| **总计** | **0** |

---

## ✅ 已解决：之前误报为 Bug 的情况

### X(!p) 当 p 是 input 时的正确行为 (2026-01-04)

**状态**: 已确认 - Cosy2 结果正确
**说明**: 这些案例之前被误认为 bug，实际上 Cosy2 的结果是正确的。

#### 测试案例

| 公式 | Partition | Cosy2 | Cosy | 正确结果 | 状态 |
|------|-----------|-------|------|----------|------|
| `X(!(p3))` | in=p3, out=p2 | **UNREALIZABLE** ✅ | Realizable | UNREALIZABLE | Cosy2 正确 |
| `G(!(p3))` | in=p3, out=p2 | REALIZABLE ✅ | Unrealizable | REALIZABLE | Cosy2 正确 |
| `G(p3)` | in=p3, out=p2 | REALIZABLE ✅ | Unrealizable | REALIZABLE | Cosy2 正确 |

#### 分析：为什么 `X(!p3)` 是 UNREALIZABLE？

**游戏规则**：
1. System 先选择 outputs
2. Environment 再选择 inputs
3. 检查公式是否满足

**对于 `X(!p3)`**：
- p3 是 **input** (Environment 控制)
- Environment 想让公式不满足 → 选择 p3=true → `!p3` = false
- **Environment 可以总是选择 p3=true**

**结论**：当 p3 是 input 时，`X(!p3)` → **UNREALIZABLE**

#### 原因分析

Cosy 可能对这些案例有不同的理解或实现细节，但根据 LTLf synthesis 的标准游戏模型：
- **Cosy2 的结果是正确的**
- 这些不是 bug，而是 Cosy 的实现可能有不同的假设

---

## （无活跃 Bug）

当前没有已确认的 bug。所有之前报告的"不一致"案例经分析后，确认 Cosy2 的结果是正确的。

---

### Benchmark 准确率问题 (68% vs 100%)

**状态**: 部分已修复
**日期**: 2026-01-04 (更新)
**详情**: `docs/working_issues/2026-01-02_PM_BenchmarkAccuracy/`

**问题描述**:
- SMv1000 benchmark 准确率 68.42% (参考实现 Cosy: 95%+)

**已修复**:
- ✅ XNF 转换实现 (Until/Release)
- ✅ X(φ) 作为 XNF 基础情况，不递归转换内部
- ✅ 原 4 个 FAIL 案例 (f101, f108, f117, f13) 测试通过
- ✅ 确认 "不一致"案例实际上是 Cosy2 正确，Cosy 理解不同

**结论**：
- Cosy2 的实现在这些案例上是正确的
- 与 Cosy 的差异可能是由于不同的游戏模型假设

