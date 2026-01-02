# ADR 004: 异常层次设计

> 状态: 已接受 (2026-01-02)

---

## 上下文

公式处理过程中可能出现多种错误情况：
- 解析错误（语法错误）
- 验证错误（未声明变量）
- 运行时错误（内存不足等）

需要一个清晰的异常处理机制来区分不同类型的错误。

---

## 决策

采用**自定义异常层次结构**，继承自 `std::runtime_error`：

```cpp
namespace formula {

// 基类 - 所有公式相关异常
class FormulaException : public std::runtime_error {
public:
    explicit FormulaException(const std::string& msg)
        : std::runtime_error(msg) {}
};

// 解析错误 - 语法问题
class ParseException : public FormulaException {
public:
    explicit ParseException(const std::string& msg)
        : FormulaException("Parse error: " + msg) {}
};

// 验证错误 - 语义问题
class ValidationException : public FormulaException {
public:
    explicit ValidationException(const std::string& msg)
        : FormulaException("Validation error: " + msg) {}
};

// 未声明变量 - 具体验证错误
class UndeclaredVariableException : public ValidationException {
public:
    explicit UndeclaredVariableException(const std::string& name)
        : ValidationException("Undeclared variable: " + name) {}
};

} // namespace formula
```

---

## 理由

1. **类型安全**: 不同异常类型可分别捕获
2. **信息保留**: 基类自动添加错误前缀
3. **标准兼容**: 继承 `std::runtime_error`，与标准库异常一致
4. **可扩展**: 未来可添加新的异常类型

---

## 后果

**调用方可选择性捕获**:
```cpp
try {
    pool.parse(input);
} catch (const UndeclaredVariableException& e) {
    // 处理未声明变量
} catch (const ParseException& e) {
    // 处理语法错误
} catch (const FormulaException& e) {
    // 处理所有公式异常
}
```

**已实现位置**:
- `src/formula/formula_parser.cpp` - ParseException
- `src/formula/formula_pool.cpp` - UndeclaredVariableException
