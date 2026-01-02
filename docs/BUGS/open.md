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

### Benchmark 准确率问题 (64% vs 100%)

**状态**: 部分修复中
**日期**: 2026-01-02
**详情**: `docs/working_issues/2026-01-02_PM_AccuracyIssue/`

**问题描述**:
- SMv1000 benchmark 准确率 64% (参考实现 Cosy: 100%)
- 已修复: X(input), G(input) 的输入依赖检查
- 待验证: 完整 benchmark 结果

**已修复**:
- ✅ Next 公式输入依赖检查
- ✅ Release 公式输入依赖检查
- ✅ 终端状态分类逻辑
- ✅ 失败输入字面量处理

**下一步**:
- 运行完整 SMv1000 benchmark 验证准确率

