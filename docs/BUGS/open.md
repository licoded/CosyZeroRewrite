# Open Bugs / Known Issues

> 未修复的 Bug 和已知限制

---

## 统计

| 优先级 | 数量 |
|--------|------|
| 🔴 高 | 1 |
| 🟡 中 | 0 |
| 🟢 低 | 0 |
| **总计** | **1** |

---

## 🔴 高优先级

### Benchmark 准确率问题 (68% vs 100%)

**状态**: 调查中
**日期**: 2026-01-02 (更新)
**详情**: `docs/working_issues/2026-01-02_PM_BenchmarkAccuracy/`

**问题描述**:
- SMv1000 benchmark 准确率 68.42% (参考实现 Cosy: 95%+)
- 小范围测试 (20个案例): 13 passed, 6 failed
- 失败类型: 5 False Positives, 1 False Negative

**已修复**:
- ✅ XNF 转换实现 (Until/Release)
- ✅ OnTheFlyDFA 集成 XNF 转换
- ✅ G p1 测试用例修复 (Release with false 语义)

**待调查**:
- 复杂嵌套公式 (如 `p7 R ((!p3 R X(!p4)) U p0)`) 结果不正确
- 可能需要进一步调整转移生成逻辑

**失败案例**:
- f102: MISMATCH (expected R, got U) - False Negative
- f103, f104, f112, f114, f115: MISMATCH (expected U, got R) - False Positives

**下一步**:
- 分析 XNF 转换后的转移生成逻辑
- 检查 Release/Until 嵌套情况的处理

