# 测试报告

**日期**: 2026-01-03
**时间**: 23:15
**Git commit**: da91680

---

## 一、单元测试结果

### 测试执行命令
```bash
cd build && make test
```

### 测试结果汇总

| 测试名称 | 状态 | 用时 | 说明 |
|---------|------|------|------|
| formula_tests | ✅ PASSED | 0.00s | Formula 基础功能测试 |
| parser_checker_tests | ✅ PASSED | 0.00s | 解析器和检查器测试 |
| transformation_tests | ✅ PASSED | 1.47s | 公式转换测试 (196个公式) |
| dfa_tests | ✅ PASSED | 0.00s | DFA 构造测试 |
| synthesis_tests | ✅ PASSED | 0.00s | Synthesis 核心测试 |
| on_the_fly_synthesis_tests | ❌ FAILED | 0.01s | On-the-fly synthesis 测试 |
| tarjan_scc_tests | ✅ PASSED | 0.00s | Tarjan SCC 算法测试 |
| io_separation_test | ✅ PASSED | 0.00s | I/O 分离测试 |
| strategy_extraction_test | ❌ ABORTED | 0.06s | 策略提取测试 (core dump) |

**总体通过率**: 7/9 = 77.8%

---

## 二、失败的测试分析

### 2.1 on_the_fly_synthesis_tests

**结果**: 6/12 测试通过，6个失败

**失败的测试**:
- 部分公式在 on-the-fly synthesis 中返回了错误的结果
- 这些测试涉及复杂的时序公式（Until, Release, Eventually 等）

**可能原因**:
- ESA (Empty String Accepting) 逻辑可能需要进一步调整
- Game graph 的 SCC 分类逻辑可能存在问题

### 2.2 strategy_extraction_test

**错误**: Assertion failure + core dump

**具体错误**:
```
Formula: X(v0)
Expected: Realizable
Actual: NOT realizable
```

**分析**:
- `X(v0)` (Next p1) 公式应该是 realizable 的
- 但当前实现返回 NOT realizable
- 这是已知的边界情况问题

---

## 三、Benchmark 测试

### 3.1 并发测试（原 benchmark_runner）

**测试命令**:
```bash
./build/benchmark_runner benchmarks/sm1000 1 50
```

**结果**: ❌ 超时
- 即使只运行 3 个测试用例也会超时（30秒+）
- 说明某些测试用例存在严重的性能问题或并发问题

### 3.2 串行测试（新增 serial_benchmark.sh）

**测试命令**:
```bash
./scripts/serial_benchmark.sh benchmarks/sm1000 5
```

**结果**:
- 测试了 5 个公式（f100-f104）
- 3 个返回 error（退出码1，但实际有结果输出）
- 2 个超时（60秒超时）

**发现**:
1. **Cosy2 退出码问题**: Cosy2 即使成功返回结果也返回退出码 1
   - 这导致脚本将所有测试误判为 "error"
   - 实际上 f100 返回了 UNREALIZABLE 结果

2. **超时问题**: f103, f104 在 60 秒内未完成
   - 这些公式可能过于复杂，存在状态爆炸问题

**f100 测试详情**:
```
Formula: (!(p0)) | ((!(p2)) U (!(p7)))
Expected: Realizable
Actual: UNREALIZABLE (不匹配)
Time: ~10ms
```

---

## 四、日志目录结构验证

### 当前结构
```
logs/
├── formula/2026-01-03/03-evening/formula_20260103_HHMMSS.log
├── tableau/2026-01-03/03-evening/tableau_debug_20260103_HHMMSS.log
├── benchmark/2026-01-03/03-evening/benchmark_20260103_HHMMSS.log
└── failures/2026-01-03/04-night/failures.log

results/
├── benchmark/2026-01-03/03-evening/benchmark_results_20260103_HHMMSS.csv
└── benchmark/serial/2026-01-03/serial_benchmark_20260103_HHMMSS.csv
```

**状态**: ✅ 日志目录结构正确，所有日志都保存到项目根目录下的 `logs/` 和 `results/` 目录

---

## 五、新增改进

### 5.1 串行测试脚本

创建了 `scripts/serial_benchmark.sh` 用于串行测试：
- 一次只测试一个公式，避免并发问题
- 逐步增加测试数量，便于定位问题
- 结果保存在 `results/benchmark/serial/` 目录

### 5.2 CLAUDE.md 更新

记录了 Benchmark 超时问题的解决方案：
- 使用串行模式测试
- 逐步增加测试数量

---

## 六、总结

### ✅ 正常功能
- Formula 模块 (NNF, XNF, Simplify)
- Parser 模块
- Transformation 模块
- DFA/Tableau 基础功能
- Synthesis 核心算法
- Tarjan SCC 算法
- I/O 分离
- 日志目录结构

### ❌ 存在问题
1. **On-the-fly synthesis**: 部分复杂时序公式结果不正确
2. **Strategy extraction**: X(v0) 这类边界情况处理错误
3. **性能问题**: 某些测试用例会导致严重超时
4. **Benchmark 准确率**: f100 结果与预期不匹配

### 建议
1. 修复 Cosy2 退出码问题（成功时应返回 0）
2. 添加性能监控和超时机制
3. 使用串行测试脚本逐步验证功能正确性
4. 优先修复简单公式的 synthesis 结果

---

**报告生成时间**: 2026-01-03 23:15
