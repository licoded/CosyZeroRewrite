# 架构文档

> CosyZeroRewrite 项目架构与技术细节

## 目录

| 文档 | 说明 |
|------|------|
| [Synthesis](./synthesis.md) | **LTLf Synthesis 完整架构** (算法流程、版本对比) |
| [Pipeline](./pipeline.md) | LTLf Synthesis 完整流水线 |
| [Components](./components.md) | 核心组件设计 (Formula, FormulaPool, TableauState) |
| [Algorithms](./algorithms.md) | 算法复杂度分析 |
| [Dependencies](./dependencies.md) | 外部依赖和版本要求 |
| [Roadmap](./roadmap.md) | 实现路线图 |
| [ADR](./adr/) | 架构决策记录 (Architecture Decision Records) |

## 快速概览

```
LTLf Formula → Parser → Preprocessing → Tableau DFA → Game Solving → Strategy
```

**设计原则**:
- **不可变性**: Formula 对象一旦创建不可修改
- **Hash Consing**: 结构相同的公式只存储一份
- **RAII**: FormulaPool 管理所有内存
- **显式优先**: 先实现显式状态，BDD 符号化后续优化
