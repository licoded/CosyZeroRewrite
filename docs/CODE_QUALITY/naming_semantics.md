# 命名语义与准确性分析

**优先级**: **CRITICAL** - 命名是代码可读性的基础，错误的命名会导致误解和 bug

---

## Critical 问题

### 1. `create_end()` - 命名不准确

**位置**: `include/automata/dfa.hpp:230-245`

```cpp
/** @brief Create or get End marker (singleton) */
Formula* create_end();
```

**问题描述**:
- 名称 `create_end()` 暗示创建"结束标记"
- 实际语义：创建表示"有限轨迹最后位置"的特殊原子命题
- 在 LTLf 语义中，这是一个特殊的原子命题，表示"这是轨迹的最后一个位置"
- "End" 可能被误解为"结束这个过程"或"终止"

**影响**:
- 开发者可能误解其用途
- 与 `create_last_position()` 相比语义不够清晰

**建议修复**:
```cpp
// 选项 A: 更明确的名称
Formula* create_end_marker();

// 选项 B: 直接描述语义
Formula* create_last_position();

// 选项 C: 强调这是特殊原子命题
Formula* create_end_proposition();
```

**推荐**: `create_end_marker()` - 平衡了清晰度和简洁性

---

### 2. `is_realizable()` - 占位符实现误导

**位置**: `include/synthesis/synthesis.hpp:72-78`, `src/synthesis/synthesis.cpp:76-85`

```cpp
std::optional<bool> is_realizable(formula::Formula* f);
```

**问题描述**:
- 函数名和接口看起来是完整实现
- 实际只是返回 `std::nullopt` 的占位符
- 注释没有说明这是未实现的占位符

**当前实现**:
```cpp
std::optional<bool> Synthesis::is_realizable(formula::Formula* f) {
    // Use the new pure C++ GameSolver implementation
    // For now, use default variable partitioning
    std::vector<std::string> output_vars;
    std::vector<std::string> input_vars;

    // TODO: Extract variables from formula and partition them
    // For now, return unknown
    (void)f;          // These parameters are unused
    (void)output_vars;
    (void)input_vars;
    return std::nullopt;
}
```

**影响**:
- 调用者可能误以为功能已实现
- 测试可能"通过"但实际上没有测试任何东西

**建议修复**:
```cpp
// 选项 A: 重命名明确表示这是占位符
[[nodiscard]] std::optional<bool> is_realizable_placeholder(formula::Formula* f);

// 选项 B: 添加明确的注释并标记为未实现
// TODO: NOT IMPLEMENTED - Returns std::nullopt until GameSolver integration is complete
[[nodiscard]] std::optional<bool> is_realizable(formula::Formula* f);

// 选项 C: 完全移除直到实现
// （从头文件中移除，仅在开发分支保留）
```

**推荐**: 选项 B - 添加明确的 `[[deprecated]]` 或 `// TODO: NOT IMPLEMENTED` 注释

---

## High 问题

### 3. `xnf_with_tail` - "tail" 术语不标准

**位置**: `include/formula/formula.hpp:143`

```cpp
Formula* xnf_with_tail(FormulaPool& pool) const;
```

**问题描述**:
- "tail" 不是 LTLf 文献中的标准术语
- 实际语义：转换为 XNF 后附加 End 标记
- 可能被误解为"尾部"或"剩余部分"

**建议修复**:
```cpp
Formula* xnf_with_end_marker(FormulaPool& pool) const;
```

---

## Medium 问题

### 4. `rmnext()` - 缩写不清晰

**位置**: `include/formula/formula.hpp:157`

```cpp
Formula* rmnext(FormulaPool& pool, Formula* edge,
                const std::unordered_set<int>& all_vars) const;
```

**问题描述**:
- `rmnext` 是 "remove next" 的缩写
- 不直观，需要查看实现才能理解
- 实际语义是 LTLf progression

**建议修复**:
```cpp
// 选项 A: 完整单词
Formula* remove_next(FormulaPool& pool, Formula* edge,
                     const std::unordered_set<int>& all_vars) const;

// 选项 B: 使用标准术语
Formula* progression(FormulaPool& pool, Formula* edge,
                     const std::unordered_set<int>& all_vars) const;
```

**推荐**: `progression()` - 使用 LTLf 文献的标准术语

---

### 5. `get_precedence()` - 应为内部函数

**位置**: `src/formula/formula.cpp:31-51`

```cpp
static int get_precedence(Formula::OpType op) {
    switch (op) {
        case Formula::OpType::Or: return 1;
        case Formula::OpType::And: return 2;
        // ...
    }
}
```

**问题描述**:
- 函数名正确，但应该是类的静态成员或 constexpr 数组
- 当前作为文件静态函数，不易维护

**建议修复**:
```cpp
// 在类中定义为静态 constexpr 数组
class Formula {
private:
    static constexpr int PRECEDENCE[] = {
        0,  // None
        4,  // Not
        2,  // And
        1,  // Or
        3,  // Until
        3,  // Release
        4,  // Next
        5,  // Literal
        5,  // True
        5,  // False
    };
};
```

---

## 命名最佳实践建议

1. **使用完整单词**：避免缩写（如 `rmnext` → `remove_next` 或 `progression`）
2. **使用领域术语**：使用 LTLf/formula 文献中的标准术语
3. **避免误导性名称**：确保名称准确反映语义
4. **布尔值命名**：使用 `is_`, `has_`, `should_` 等前缀
5. **一致性**：相似概念使用相似的命名模式

---

## 命名检查清单

- [ ] 函数名是否准确描述其行为？
- [ ] 参数名是否清晰传达其用途？
- [ ] 变量名是否有歧义？
- [ ] 是否使用了领域标准术语？
- [ ] 布尔值命名是否清晰？
- [ ] 类名是否反映其职责？
