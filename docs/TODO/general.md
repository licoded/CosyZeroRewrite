# General TODO

> 通用待办事项（不特定于任何模块）

---

## 🟡 中优先级

### [#001] 添加集成测试

**状态**: 待办
**预计时间**: 4 小时

**描述**:
添加端到端的集成测试，测试完整的 synthesis 流程。

**子任务**:
- [ ] 设计测试用例
- [ ] 实现 test harness
- [ ] 添加到 CI/CD

---

### [#002] 添加性能基准测试

**状态**: 待办
**预计时间**: 4 小时

**描述**:
建立性能基准，跟踪优化效果。

**子任务**:
- [ ] 选择基准公式集
- [ ] 实现基准测试框架
- [ ] 建立性能历史

---

### [#005] 实现缺失的 simplify_until() 规则 (2026-01-10)

**状态**: 待办
**预计时间**: 2 小时
**相关文档**: `docs/ARCHITECTURE/migration_docs/formula_module/SIMPLIFY_IMPLEMENTATION.md`

**描述**:
原 aalta 实现中的 simplify_until() 有 13 条规则，新实现只实现了 8 条。
需要补充以下 5 条缺失规则：

| 规则 | 公式 | 结果 | 说明 |
|------|------|------|------|
| 7 | `φ U (ψ R φ)` | `ψ R φ` | Release conversion |
| 8 | `(ψ R φ) U φ` | `φ` | Release absorption |
| 9 | `(φ U ψ) U φ` | `ψ U φ` | Associativity variant |
| 10 | `(ψ U φ) U φ` | `ψ U φ` | Duplicate right |
| 13 | `φ U FG(ψ)` | `FG(ψ)` | FG detection (liveness) |

**子任务**:
- [ ] 实现 Release conversion: `φ U (ψ R φ) → ψ R φ`
- [ ] 实现 Release absorption: `(ψ R φ) U φ → φ`
- [ ] 实现 Associativity: `(φ U ψ) U φ → ψ U φ`
- [ ] 实现 Duplicate right: `(ψ U φ) U φ → ψ U φ`
- [ ] 实现 FG detection (需要 FG/FG 判断函数)

**优先级说明**: 这些规则可以进一步简化公式，提升性能。但当前系统已能正确工作，非紧急。

---

### [#006] 实现缺失的 simplify_release() 规则 (2026-01-10)

**状态**: 待办
**预计时间**: 2 小时
**相关文档**: `docs/ARCHITECTURE/migration_docs/formula_module/SIMPLIFY_IMPLEMENTATION.md`

**描述**:
原 aalta 实现中的 simplify_release() 有 12 条规则，新实现只实现了 6 条。
需要补充以下 6 条缺失规则：

| 规则 | 公式 | 结果 | 说明 |
|------|------|------|------|
| 6 | `φ R (φ R ψ)` | `φ R ψ` | Idempotent |
| 7 | `φ R (ψ R φ)` | `ψ R φ` | Commutativity |
| 8 | `φ R (ψ U φ)` | `ψ U φ` | Until conversion |
| 9 | `(ψ U φ ∨ ...) R φ` | `φ` | Until absorption |
| 10 | `(φ R ψ) R φ` | `ψ R φ` | Associativity |
| 11 | `(ψ R φ) R φ` | `ψ R φ` | Duplicate right |

**子任务**:
- [ ] 实现 Idempotent: `φ R (φ R ψ) → φ R ψ`
- [ ] 实现 Commutativity: `φ R (ψ R φ) → ψ R φ`
- [ ] 实现 Until conversion: `φ R (ψ U φ) → ψ U φ`
- [ ] 实现 Until absorption: `(ψ U φ ∨ ...) R φ → φ`
- [ ] 实现 Associativity: `(φ R ψ) R φ → ψ R φ`
- [ ] 实现 Duplicate right: `(ψ R φ) R φ → ψ R φ`

**优先级说明**: 这些规则可以进一步简化公式，提升性能。但当前系统已能正确工作，非紧急。

---

## 🟢 低优先级

### [#003] 完善 Doxygen 注释

**状态**: 待办
**预计时间**: 8 小时

**描述**:
为所有公共 API 添加 Doxygen 格式注释。

**子任务**:
- [ ] Formula 类注释
- [ ] FormulaPool 类注释
- [ ] Synthesis 模块注释

---

### [#004] 记录设计决策

**状态**: 待办
**预计时间**: 4 小时

**描述**:
将设计决策记录到文档，方便后续维护。

**子任务**:
- [ ] 创建 ARCHITECTURE.md
- [ ] 记录关键 trade-off
- [ ] 添加 ADR (Architecture Decision Records)

---

## 统计

| 优先级 | 数量 |
|--------|------|
| 🔴 高 | 0 |
| 🟡 中 | 4 |
| 🟢 低 | 2 |
| **总计** | **6** |
