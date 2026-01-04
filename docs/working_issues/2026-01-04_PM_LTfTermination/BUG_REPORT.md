# LTLf 终止状态判断错误

**日期**: 2026-01-04 下午
**状态**: Open
**优先级**: High

---

## 问题描述

当前代码错误地将 `next_phi = true` 视为强制终止状态，导致某些 edge 没有被正确记录。

### 症状

在公式 `(true U p)` 的 trace 中：
- E1 状态没有生成任何 env move
- E1 被分类为 Ewin（可能不正确）

### 根本原因

**错误的假设**: `formulas().empty()` (即 `prop_atoms_.empty()`) 意味着强制终止

**正确理解**:
1. **到达 `true` 不是强制结束** → 游戏可以选择继续或结束
2. **包含 `End` 的 edge 才是强制结束** → 游戏必须结束
3. **只能被 empty trace/空串接收**，没有其他通向 sys move Swin 的方式 → 强制结束

### LTLf 语义澄清

| 状态 | 含义 | 行为 |
|------|------|------|
| `next_phi = true` | 公式被满足 | **可选终止**，可以继续 |
| `next_phi = false` | 公式被违反 | 可选终止 |
| `next_phi = End` | 明确的终止标记 | **强制终止** |
| `prop_atoms = {}` | 无命题原子 | **不等于终止**！ |

### 需要修改的地方

1. **函数命名问题**
   - `formulas()` 实际返回 `prop_atoms_`，名称误导
   - 应该重命名为 `prop_atoms()` 或类似名称

2. **终止状态判断错误** (`on_the_fly_solver.cpp:295`)
   ```cpp
   // 错误：将 prop_atoms 为空视为终止
   if (next_dfa->formulas().empty()) {
       // Terminal DFA state reached - game ends here
       // Don't add a successor
   }
   ```

   应该改为：
   ```cpp
   // 正确：只有明确包含 End 才是强制终止
   // true/false 不是终止状态，需要继续转移
   ```

3. **转移记录要求**
   - **到 `true` 的转移要记录**
   - **`true` 之后的转移也要记录**
   - 不能因为到达 true 就停止扩展

---

## 技术细节

### 当前代码问题位置

**文件**: `src/synthesis/on_the_fly_solver.cpp`
**行号**: 293-304

```cpp
// Check if the next DFA state is terminal (empty formula set)
// In LTLf, an empty state means the game has ended
if (next_dfa->formulas().empty()) {
    // Terminal DFA state reached - game ends here
    // Don't add a successor (terminal game state)
    // The winner will be determined by whether the terminal DFA state is accepting
    LOG_DEBUG("OnTheFlyGameSolver: terminal DFA state reached, not adding successor");
} else {
    // Create system state with the input assignment stored
    succs.push_back(system_state(next_dfa, in));
}
```

### `formulas()` 函数问题

**文件**: `include/automata/tableau.hpp:120`

```cpp
/**
 * @brief Get all formulas in this state (backward compatibility alias for prop_atoms)
 */
const FormulaSet& formulas() const { return prop_atoms_; }
```

这个函数名 `formulas()` 误导性地让人以为返回"所有公式"，实际返回的是"命题原子集"。

---

## 修复计划

### Phase 1: 重命名函数
- [ ] `formulas()` → `prop_atoms()` (更准确的名称)
- [ ] 更新所有调用点

### Phase 2: 修复终止状态判断
- [ ] 移除 `formulas().empty()` 的终止检查
- [ ] 添加正确的 `End` 检测
- [ ] 确保 `true` 状态的转移被正确记录

### Phase 3: 测试验证
- [ ] 用 `(true U p)` 验证 E1 有正确的 env move
- [ ] 验证终止状态判断的正确性
- [ ] 运行完整测试套件

---

## 参考资料

- LTLf semantics: `true` 是一个可接受状态，不是终止状态
- AAAI2019 论文对终止的定义
- Tableau 状态转移的正确处理
