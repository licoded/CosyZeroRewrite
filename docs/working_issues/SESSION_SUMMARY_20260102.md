# 会话总结 - 2026-01-02

## 当前状态

### 已完成工作

1. **XNF 转换实现** (`src/formula/xnf.cpp`)
   - Until: `xnf(φ₁ U φ₂) = xnf(φ₂) ∨ (xnf(φ₁) ∧ X(φ₁ U φ₂))`
   - Release: `xnf(φ₁ R φ₂) = xnf(φ₂) ∧ (xnf(φ₁) ∨ X(φ₁ R φ₂))`
   - 不使用显式 End 标记

2. **OnTheFlyDFA 集成** (`src/automata/tableau.cpp`)
   - 在 NNF 后应用 XNF 转换
   - 修复 `is_accepting` 和 `is_locally_consistent` 检查 Release with false

3. **文档更新**
   - XNF 论文下载到 `docs/papers/xnf_paper.pdf`
   - XNF 转换规则文档 `docs/ARCHITECTURE/xnf_detailed.md`
   - 空串接受性表格：X, U, F 不能空串接受；WX, R, G 可以

4. **单元测试**: 全部通过 (10/10)

### 当前问题

**Benchmark 准确率 68.42%** (小范围测试 20 案例)
- 13 passed, 6 failed
- 5 False Positives (应该是 U，我们说 R)
- 1 False Negative (应该是 R，我们说 U)

**失败案例**:
```
f102: MISMATCH (expected R, got U)
f103, f104, f112, f114, f115: MISMATCH (expected U, got R)
```

**示例失败公式**:
```
f104: (p7) R (((!(p3)) R (X(!(p4)))) U (p0))
```
XNF 后: `((v3 | ((X(!v2) & (!v1 | X((!v1 R X(!v2))))) & X(((!v1 R X(!v2)) U v3)))) & (v0 | X((v0 R ((!v1 R X(!v2)) U v3)))))`
我们的结果: REALIZABLE
期望: UNREALIZABLE

### 下一步方向

1. **分析转移生成逻辑**
   - XNF 转换后的公式结构复杂
   - 需要检查 `next()` 函数如何处理 XNF 转换后的公式

2. **检查嵌套 Release/Until 处理**
   - 当前失败案例都包含嵌套的 Release 和 Until
   - 可能需要特殊处理

3. **关键文件**
   - `src/automata/tableau.cpp` - TableauState::next(), initial()
   - `src/formula/xnf.cpp` - XNF 转换

### Git 状态

```
Branch: dev
Status: Clean (all committed)
Latest commits:
- 75c718b docs: add test workflow specification
- bda737e feat: implement XNF transformation
- af2764f docs: clarify XNF transformation
```

### 测试流程（已记录）

1. 单元测试: `make test`
2. 小范围抽查: `./build/benchmark_runner benchmarks/sm1000 1 20`
3. 全量测试: `./build/benchmark_runner benchmarks/sm1000 1 1000`

### 核心概念（已确认）

- **X, U, F**: 不能空串接受 → 必须继续
- **WX, R, G**: 可以空串接受 → 可以结束
- **XNF 转换** 不使用显式 End 标记，空串判断在转移生成时进行
