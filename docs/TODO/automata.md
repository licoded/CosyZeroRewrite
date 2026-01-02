# Automata Module TODO

> Automata/Tableau 模块待办事项

---

## 统计

| 优先级 | 数量 |
|--------|------|
| 🔴 高 | 0 |
| 🟡 中 | 0 |
| 🟢 低 | 1 |
| **总计** | **1** |

---

## 🟢 低优先级

### [哈希碰撞率统计检测](./automata.md#hash-collision-stats)

**标签**: `performance` `hashing` `monitoring`

**描述**:

为 TableauState 和 FormulaPool 的哈希函数添加运行时碰撞率统计，验证 Boost 风格组合哈希函数在实际场景中的表现。

**实现方案**:

```cpp
// 在 FormulaPool 或 TableauState 中添加统计
class HashCollisionStats {
    size_t total_hash_computations_ = 0;
    size_t hash_collisions_ = 0;
    // ...

    void record_collision() { hash_collisions_++; }
    double collision_rate() const {
        return total_hash_computations_ > 0
            ? static_cast<double>(hash_collisions_) / total_hash_computations_
            : 0.0;
    }
};
```

**预期结果**:
- 碰撞率应该 < 0.01%
- 如果碰撞率过高，需要重新评估哈希函数

**相关代码**:
- `src/automata/tableau.cpp`: `compute_formula_set_hash()`
- `include/formula/formula_pool.hpp`: FormulaHash, FormulaEqual

---

## 归档

> 已完成的待办事项
