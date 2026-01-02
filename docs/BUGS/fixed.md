# Fixed Bugs / Resolved Issues

> 已修复的 Bug 和已解决的问题

---

## [Bug #001] TableauState 接受状态条件错误

**修复日期**: 2026-01-02
**影响版本**: v1.0

**问题**:
`is_accepting()` 对 Until 公式的检查不正确：
```cpp
// 错误：只要 ψ₁ 或 ψ₂ 有一个在 Γ 中就接受
if (formulas_.count(right) == 0 && formulas_.count(left) == 0)
    return false;
```

**修复**:
在 LTLf 中，`ψ₁ U ψ₂` 要求 ψ₂ 必须最终为真。如果状态中有 `ψ₁ U ψ₂` 但 ψ₂ 不在状态中，说明 Until 还在等待，不能是接受状态。
```cpp
// 正确：必须有 ψ₂ 才能接受
if (formulas_.count(right) == 0)
    return false;  // Until still waiting for right side
```

**相关提交**: `97e2f8f` - fix: correct accepting state condition for Until formulas

---

## 统计

| 年份 | 修复数量 |
|------|---------|
| 2026 | 1 |
| **总计** | **1** |
