# 调试会话总结 - 2026-01-03

## 当前状态

**准确率**: 91.84% (45/49 通过)
- False Positives: 4 个
- False Negatives: 0 个

## 重要发现

### 错误的调试方向

之前的调试方向有误：一直在添加特殊情况处理（ad-hoc pattern detection），而不是依赖 XNF 转换的正确性。

**错误的改动**：
- `A & (B | !A)` 模式检测
- Release 左边输入依赖的"特殊处理"
- 各种复杂的嵌套检查

### 正确的原则

根据用户指导，正确的原则是：

1. **XNF 转换已经处理了一切**
   - 所有 temporal 算子都转换为 "当前步命题 + Next 转移"
   - edge 上的命题由 XNF 自动生成
   - 不需要额外的特殊情况处理

2. **Empty Trace Accepting 是关键**
   - U、X、F: 不能在空串上满足（需要至少一个时间点）
   - R、G: 可以在空串上满足（vacuously true）
   - System move 前检查：如果状态包含 U/X/F，不能在此终止

3. **is_accepting 应该检查什么**
   - Tableau accepting: 无矛盾、局部一致
   - Synthesis accepting: 系统可以从此状态保证满足
   - 两者概念不同，不能混淆

## 剩余问题 (4 False Positives)

- f104: `(p7) R (((!(p3)) R (X(!(p4)))) U (p0))`
- f114: `F((X(X(G(!(p8))))) R ((p3) & (F(p4)) & ((p6) U (G(p5)))))`
- f115: `(X(X(p3))) R ((G(p4)) U (p0))`
- f129: `F((F(G(p4))) & (G((p5) & ((p1) | (!(p5))))))`

## 下一步方向

1. 回退不必要的特殊情况处理代码
2. 理解 XNF 转换的输出
3. 检查 empty trace accepting 规则的实现
4. 基于 XNF 和 game semantics 重新审视 is_accepting

## 已归档知识

参见: `docs/ARCHITECTURE/ltlf_synthesis_knowledge.md`
