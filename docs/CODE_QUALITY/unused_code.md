# 未使用代码分析

---

## High 问题

### 1. 占位符函数中的未使用参数

**位置**: `src/synthesis/synthesis.cpp:54-84`

```cpp
std::optional<bool> Synthesis::is_satisfiable(formula::Formula* f) {
#ifdef FORMULA_USE_Z3
    return formula::FormulaZ3::is_satisfiable(f, 0, 1000);
#else
    (void)f;  // Parameter is used but marked as unused
    return std::nullopt;
#endif
}

std::optional<bool> Synthesis::is_valid(formula::Formula* f) {
#ifdef FORMULA_USE_Z3
    return formula::FormulaZ3::is_valid(f, 0, 10000);
#else
    (void)f;  // Parameter is used but marked as unused
    return std::nullopt;
#endif
}
```

**问题描述**:
- 在非 Z3 构建路径中，参数被 `(void)f` 标记为"使用"以避免警告
- 但参数实际上没有被使用
- 这种模式不一致且令人困惑

**建议修复**:
```cpp
#ifndef FORMULA_USE_Z3
[[maybe_unused]] static std::optional<bool> is_satisfiable(formula::Formula* f) {
    return std::nullopt;
}
#endif
```

或者使用 `[[maybe_unused]]` 属性：
```cpp
std::optional<bool> Synthesis::is_satisfiable([[maybe_unused]] formula::Formula* f) {
#ifdef FORMULA_USE_Z3
    return formula::FormulaZ3::is_satisfiable(f, 0, 1000);
#else
    return std::nullopt;
#endif
}
```

---

### 2. `is_realizable()` 中的未使用变量

**位置**: `src/synthesis/synthesis.cpp:76-85`

```cpp
std::optional<bool> Synthesis::is_realizable(formula::Formula* f) {
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

**问题描述**:
- 变量被创建但从未使用
- `(void)` 标记只是抑制警告，不解决根本问题

**建议修复**:
```cpp
// 选项 A: 移除未使用的变量
std::optional<bool> Synthesis::is_realizable([[maybe_unused]] formula::Formula* f) {
    // TODO: NOT IMPLEMENTED - Returns std::nullopt until GameSolver integration is complete
    return std::nullopt;
}

// 选项 B: 实现功能
std::optional<bool> Synthesis::is_realizable(formula::Formula* f) {
    // TODO: Extract variables from formula and partition them
    FormulaPool pool;
    return GameSolver::is_realizable(f, pool);
}
```

---

### 3. 未使用的分区参数

**位置**: `src/synthesis/synthesis.cpp:86-94`

```cpp
std::optional<bool> Synthesis::is_realizable_with_partition(
    formula::Formula* f,
    const std::vector<std::string>& output_vars,
    const std::vector<std::string>& input_vars,
    formula::FormulaPool& pool) {
    (void)output_vars;  // Unused parameters
    (void)input_vars;
    return GameSolver::is_realizable(f, pool);
}
```

**问题描述**:
- `output_vars` 和 `input_vars` 参数在接口中定义但未使用
- 调用者可能期望这些参数会影响结果

**建议修复**:
```cpp
// 选项 A: 从接口中移除未使用的参数
std::optional<bool> Synthesis::is_realizable_with_partition(
    formula::Formula* f,
    formula::FormulaPool& pool) {
    return GameSolver::is_realizable(f, pool);
}

// 选项 B: 实现分区功能（如果需要）
std::optional<bool> Synthesis::is_realizable_with_partition(
    formula::Formula* f,
    const std::vector<std::string>& output_vars,
    const std::vector<std::string>& input_vars,
    formula::FormulaPool& pool) {
    // Use the provided partition
    return GameSolver::is_realizable_with_partition(f, pool, output_vars, input_vars);
}
```

---

## Medium 问题

### 4. 可能的未使用成员变量

**位置**: `src/formula/formula.cpp` 中的 `Formula` 类

**需要检查**:
- 某些成员变量可能只在特定代码路径中使用
- 建议使用工具如 `clang-tidy` 进行更全面的分析

---

## 检测方法

### 使用编译器警告
```bash
# GCC/Clang
-Wunused -Wunused-parameter -Wunused-variable

# CMake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused")
```

### 使用 clang-tidy
```bash
clang-tidy --checks='-*,modernize-*' -p build/ src/*.cpp
```

### 使用 cppcheck
```bash
cppcheck --enable=all --inconclusive src/
```

---

## 清理建议

1. **立即清理**: 明确的死代码（注释掉的代码块）
2. **使用 `[[maybe_unused]]`**: 保留参数但标记为可能未使用
3. **重构接口**: 移除不必要的参数
4. **实现功能**: 将占位符实现为真正的功能
