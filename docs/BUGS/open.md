# Open Bugs / Known Issues

> 未修复的 Bug 和已知限制

---

## 统计

| 优先级 | 数量 |
|--------|------|
| 🔴 高 | 2 |
| 🟡 中 | 0 |
| 🟢 低 | 0 |
| **总计** | **2** |

---

## 🔴 高优先级

### Bug #001: X(!(p)) with outputs 判断错误 (2026-01-04)

**状态**: 新发现
**影响**: Next + Not 操作符在有 outputs 时判断错误

#### 测试案例

| 项目 | 值 |
|------|-----|
| **公式** | `X(!(p3))` |
| **Partition** | `.inputs: p3`<br>`.outputs: p2` |
| **Cosy2 结果** | **UNREALIZABLE** ❌ |
| **Cosy 结果 (预期)** | **Realizable** ✅ |
| **类型** | False Negative |

#### 分析

公式 `X(!(p3))` 的语义：
- "下一时刻 p3 不为真"
- 当有 output (p2) 时，System 可以控制 p2
- Environment 只能控制 p3

Cosy2 判断为 UNREALIZABLE，但实际上 Environment 无法永远阻止 `!p3`（因为 p3 是 Environment 的变量，Environment 可以选择让 p3 为 false）。

#### Trace 文件

```
Trace: /tmp/bug_trace_x_not_p3/trace_20260104_225920.json
```

游戏图结构：
- S0 (init, System, phi=X(!p3)) → E0 (Environment)
- E0 → S1 (System, phi=!p3)
- S1 → E1 (Environment, phi=!p3)
- E1 → S2 (env={}) / S3 (env={p3})

#### 测试命令

```bash
.inputs: p3
.outputs: p2

Cosy2 "X(!(p3))" -p part      # UNREALIZABLE
Cosy "X(!(p3))" part 0       # Realizable
```

#### 相关文件

- `src/synthesis/on_the_fly_solver.cpp` - 游戏求解
- `src/automata/tableau.cpp` - 状态扩展
- `src/automata/progression.cpp` - Progression 计算

---

### Bug #002: F/G 操作符实现问题 (2026-01-04)

**状态**: 新发现
**影响**: F (Eventually) 和 G (Globally) 操作符结果与 Cosy 不一致

#### 不一致案例

| 公式 | Partition | Cosy2 | Cosy | 类型 |
|------|-----------|-------|------|------|
| `G(!(p3))` | in=p3, out=p2 | REALIZABLE | Unrealizable | False Positive |
| `G(p3)` | in=p3, out=p2 | REALIZABLE | Unrealizable | False Positive |
| `X(F(!(p3)))` | in=p3, out=p2 | UNREALIZABLE | Realizable | False Negative |

#### 测试命令

```bash
.inputs: p3
.outputs: p2

G(!(p3))      # Cosy2: REALIZABLE, Cosy: Unrealizable
G(p3)         # Cosy2: REALIZABLE, Cosy: Unrealizable
X(F(!(p3)))   # Cosy2: UNREALIZABLE, Cosy: Realizable
```

#### 分析方向

1. **F/G 的 NNF 转换**:
   - `F(φ) = true U φ`
   - `G(φ) = false R φ`
2. **F/G 的 progression 实现**
3. **空串接受性判断**:
   - F 不能接受空串（必须继续直到满足）
   - G 可以接受空串（当右侧满足时可以结束）

#### 相关文件

- `src/formula/nnf.cpp` - NNF 转换
- `src/formula/xnf.cpp` - XNF 转换
- `src/automata/progression.cpp` - Progression

---

### Benchmark 准确率问题 (68% vs 100%)

**状态**: 部分已修复
**日期**: 2026-01-04 (更新)
**详情**: `docs/working_issues/2026-01-02_PM_BenchmarkAccuracy/`

**问题描述**:
- SMv1000 benchmark 准确率 68.42% (参考实现 Cosy: 95%+)
- 小范围测试 (20个案例): 13 passed, 6 failed
- 失败类型: 5 False Positives, 1 False Negative

**已修复**:
- ✅ XNF 转换实现 (Until/Release)
- ✅ X(φ) 作为 XNF 基础情况，不递归转换内部
- ✅ 原 4 个 FAIL 案例 (f101, f108, f117, f13) 测试通过

**待调查**:
- F/G 操作符实现问题（见上方新 bug）
- 复杂嵌套公式结果不正确

