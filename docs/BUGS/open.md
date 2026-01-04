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

### F/G 操作符实现问题 (2026-01-04)

**状态**: 新发现
**影响**: F (Eventually) 和 G (Globally) 操作符结果与 Cosy 不一致

**不一致案例**:

| 公式 | Cosy2 | Cosy | 说明 |
|------|-------|------|------|
| `X(!(p3))` | UNREALIZABLE | Realizable | Next + Not |
| `G(!(p3))` | REALIZABLE | Unrealizable | Globally + Not |
| `G(p3)` | REALIZABLE | Unrealizable | Globally |
| `X(F(!(p3)))` | UNREALIZABLE | Realizable | Next + Eventually |

**测试命令**:
```bash
.inputs: p3
.outputs: p2

X(!(p3))      # Cosy2: UNREALIZABLE, Cosy: Realizable
G(!(p3))      # Cosy2: REALIZABLE, Cosy: Unrealizable
G(p3)         # Cosy2: REALIZABLE, Cosy: Unrealizable
```

**分析方向**:
1. F/G 的 NNF 转换: F(φ) = true U φ, G(φ) = false R φ
2. F/G 的 progression 实现
3. 空串接受性判断: F 不能接受空串，G 可以接受空串

**相关文件**:
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

