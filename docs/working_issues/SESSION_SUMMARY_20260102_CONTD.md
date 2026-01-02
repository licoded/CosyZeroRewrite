# 会话总结 (续) - 2026-01-02

## 当前状态

### 已完成工作

1. **OR 公式保留策略** - 重大重构
   - initial state 构造时保留 OR 公式本身，不展开两边
   - next() 函数中 OR 公式保持为整体，不展开
   - 修改了 `is_locally_consistent` 中的 OR 检查逻辑

2. **AND 公式展开**
   - next() 函数中添加了 AND 的处理（展开两边）

3. **改进 `!input` 检查**
   - 检查 OR 的两边是否都包含 `!input`
   - 只有两边都有 `!input` 才拒绝

### 测试结果

**准确率：78.95%** (19 个案例中 15 个通过)

```
之前：63.16% (12 passed, 7 failed)
现在：78.95% (15 passed, 4 failed)
提升：+15.79%
```

**混淆矩阵**：
- True Positives: 14 (正确 Realizable)
- True Negatives: 1 (正确 Unrealizable)
- False Positives: 4 (错误 Realizable)
- **False Negatives: 0** (全部修复！)

### 失败案例（False Positives）

| Formula | 期望 | 实际 | 问题 |
|---------|------|------|------|
| f103 | Unrealizable | Realizable | OR 两边都有输入依赖 |
| f104 | Unrealizable | Realizable | 嵌套 Release/Until |
| f114 | Unrealizable | Realizable | 复杂嵌套结构 |
| f115 | Unrealizable | Realizable | Release + Until |

### 核心问题

**f103 分析**：
```
公式：(X((G(p3)) U (X(p8)))) U ((!(p5)) & (F(p7)))
- 左边：G(p3) 需要 p3 为真（input）
- 右边：!p5 需要 p5 为假（input）
```

当前 `has_negated_input` 只检查 `!input`，没有检查 "需要 input 为真" 的情况（如 G(input)）。

### 下一步方向

1. **扩展输入依赖检查**：不仅检查 `!input`，也要检查 "需要 input 为真" 的情况
2. **统一两种检查**：`has_negated_input` 和 `check_temporal_dependencies` 可能需要合并
3. **检查 OR 两边的所有输入依赖**：无论类型

### Git 提交

- e48456f: preserve OR as choice point in state construction
- beea27b: check both sides of OR for !input

