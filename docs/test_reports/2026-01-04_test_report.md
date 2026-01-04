# 测试报告 2026-01-04

## 测试概览

**测试日期**: 2026-01-04
**测试环境**: Linux 6.5.0-18-generic
**编译器**: C++17 (CMake 3.10+)

---

## 1. 单元测试结果 (make test)

### 总体统计

| 测试套件 | 结果 | 耗时 |
|---------|------|------|
| formula_test | ✅ PASSED | 0.00s |
| transformation_test | ✅ PASSED | 1.31s |
| parser_test | ✅ PASSED | 0.00s |
| nnf_test | ✅ PASSED | 0.00s |
| xnf_test | ✅ PASSED | 0.00s |
| dfa_test | ✅ PASSED | 0.00s |
| tarjan_test | ✅ PASSED | 0.00s |
| progression_test | ✅ PASSED | 0.00s |
| synthesis_test | ✅ PASSED | 0.00s |
| **on_the_fly_test** | **❌ FAILED** | 0.00s |
| prop_atoms_test | ✅ PASSED | 0.00s |

**单元测试通过率**: 10/11 (90.9%)

### 失败测试详情: on_the_fly_test

**失败的断言** (6 个):

| # | 公式 | 期望 | 实际 | 说明 |
|---|------|------|------|------|
| 1 | `p1` | Realizable | Unrealizable | 基础原子公式 |
| 2 | `!p1` | Realizable | Unrealizable | 否定公式 |
| 3 | `X p1` | Realizable | Unrealizable | Next 公式 |
| 4 | `F p1` (true U p1) | Realizable | Unrealizable | Future/Until 公式 |
| 5 | `p1 U p2` | Realizable | Unrealizable | Until 公式 |
| 6 | `(!req) U ack` | Realizable | Unrealizable | 响应模式 |
| 7 | `p1 & X p2` | Realizable | Unrealizable | **已知问题 (SCC 分类)** |

**根因分析**:
- **SCC 分类问题**: 测试暴露了 on-the-fly solver 中 SCC (强连通分量) 分类存在系统性问题
- **False Negatives**: 多个 REALIZABLE 公式被错误判定为 UNREALIZABLE
- **已知问题**: 最后一个失败是文档中已知的 SCC 分类 bug

---

## 2. 小范围抽查测试 (50 案例)

### 测试配置

```
Benchmark: SMv1000
范围: 1-50
线程数: 6
超时策略: 1min -> 3min -> 5min
```

### 结果统计

| 指标 | 数值 | 百分比 |
|------|------|--------|
| 总测试数 | 49 | 100% |
| 通过 (PASS) | 4 | 8.16% |
| 失败 (MISMATCH) | 8 | 16.33% |
| 超时 (TIMEOUT) | 37 | 75.51% |
| 错误 (ERROR) | 0 | 0.00% |
| **总耗时** | **2220.14s** | - |

### 准确率分析

**混淆矩阵**:

|  | 预测 Realizable | 预测 Unrealizable |
|--|-----------------|-------------------|
| **实际 Realizable** | 1 (TP) | 7 (FN) |
| **实际 Unrealizable** | 1 (FP) | 3 (TN) |

**准确率**: 33.33%
**召回率 (TPR)**: 12.5% (1/8)
**精确率 (PPV)**: 50% (1/2)

### 问题分类

1. **False Negatives (7 个)**: 应该是 R 但被判定为 U
   - f100, f101, f102, f108, f109, f110, f117
   - 这些公式在 expected 结果中是 Realizable

2. **False Positives (1 个)**: 应该是 U 但被判定为 R
   - f13

3. **超时问题 (37 个)**: 性能瓶颈
   - 大量公式在 1 分钟内无法完成求解
   - 说明算法复杂度或实现效率存在严重问题

---

## 3. 扩展测试 (100 案例)

### 测试进展

测试在运行 91 个案例后被终止，因为超时率过高 (75%+)。

### 观察到的趋势

- 超时问题持续存在
- False Negatives 模式一致：多个期望为 R 的公式被判定为 U
- 只有少数公式能快速完成 (10-60ms)

---

## 4. 核心问题总结

### A. 正确性问题 (Critical)

1. **on_the_fly_test 失败**
   - 6/6 基础测试断言失败
   - 包括最简单的原子公式 `p1`
   - 说明 on-the-fly solver 的核心逻辑存在根本性问题

2. **Benchmark 准确率低**
   - 准确率仅 33%，远低于目标 70%+
   - False Negatives 占主导 (7 vs 1)

### B. 性能问题 (Critical)

1. **超时率极高**
   - 75.51% 测试超时
   - 大部分公式在 60 秒内无法求解完成

2. **可能的原因**:
   - Tableau 状态爆炸
   - SCC 计算效率低
   - 缺少剪枝优化

### C. 已知问题

- **SCC 分类问题**: 文档中已记录
- 位置: `docs/BUGS/open.md`

---

## 5. 建议

### 立即行动 (P0)

1. **修复 on_the_fly_test**
   - 从最简单的原子公式 `p1` 开始调试
   - 检查 winning region 计算逻辑
   - 验证 SCC 分类算法

2. **分析 False Negatives 模式**
   - 对比 f100-f110 等失败案例
   - 检查是否有共同特征 (如特定操作符组合)

3. **性能分析**
   - 使用 profiler 识别热点
   - 检查 Tableau 状态展开数量

### 后续工作 (P1)

1. 修复后重新运行完整测试流程
2. 建立回归测试集
3. 添加性能基准测试

---

## 6. 测试数据文件

- **CSV 结果**: `results/benchmark/2026-01-04/01-morning/benchmark_results_20260104_100601.csv`
- **日志文件**: `logs/benchmark/2026-01-04/01-morning/benchmark_20260104_100601.log`

---

*报告生成时间: 2026-01-04 10:35*
*报告生成工具: Claude Code*
