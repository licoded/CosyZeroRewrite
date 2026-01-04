# Const 正确性分析

---

## Medium 问题

### 1. 返回非 const 指针给不可变数据

**位置**: `include/formula/formula.hpp:55-57`

```cpp
Formula* left() const { return left_; }
Formula* right() const { return right_; }
int var_id() const { return var_id_; }
```

**问题描述**:
- Formula 类通过 Hash Consing 实现不可变性
- 但访问器返回非 const 指针
- 这允许调用者修改"不可变"的公式

**当前风险**:
```cpp
const Formula* f = pool.create(...);
Formula* left = f->left();  // 非 const 指针
left->some_mutation();       // 可以修改！违反不可变性
```

**建议修复**:
```cpp
const Formula* left() const { return left_; }
const Formula* right() const { return right_; }
int var_id() const { return var_id_; }
```

**迁移策略**:
- 先添加 const 修饰符
- 编译找出需要修改的调用代码
- 逐步修复调用代码

---

## Low 问题

### 2. 返回类型过于冗长

**位置**: `include/automata/dfa.hpp:67-68`

```cpp
const std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>&
formulas() const { return formulas_; }
```

**问题描述**:
- 返回类型很长，可读性差
- 每次修改容器类型都要更新这里

**建议修复**:
```cpp
// 使用类型别名
class TableauState {
public:
    using FormulaSet = std::unordered_set<PrimitiveFormula, PrimitiveFormulaHash, PrimitiveFormulaEqual>;

    const FormulaSet& formulas() const { return formulas_; }
    // ...
};
```

---

### 3. 成员函数可以标记为 const

**位置**: 多处

**问题描述**:
- 一些不修改对象状态的成员函数没有标记为 `const`
- 这限制了它们在 const 上下文中的使用

**建议检查**:
```cpp
// 如果函数不修改任何成员变量，应该标记为 const
class Example {
    int value_;

public:
    int get_value() const { return value_; }     // ✓ 正确
    void print() const { std::cout << value_; }  // ✓ 正确

    int compute() { return value_ * 2; }         // ✗ 应该是 const
};
```

---

## Const 正确性最佳实践

### 1. 默认使用 const
```cpp
// 参数
void func(const std::string& str);  // 只读参数

// 成员函数
int get_value() const;  // 不修改对象的函数

// 返回值
const std::string& name() const;  // 返回内部引用
```

### 2. const 正确传播
```cpp
class MyClass {
    std::vector<int> data_;

public:
    // 非 const 版本 - 允许修改
    std::vector<int>& data() { return data_; }

    // const 版本 - 只读访问
    const std::vector<int>& data() const { return data_; }
};
```

### 3. 返回值注意
```cpp
// 返回原生类型 - 不需要 const
int get_value() const { return value_; }

// 返回值类型 - 不需要 const（临时对象）
std::string get_name() const { return name_; }

// 返回引用 - 需要 const
const std::string& name() const { return name_; }
```

---

## 迭代器 const 正确性

```cpp
class Container {
    std::vector<int> data_;

public:
    // 非 const 迭代器
    using iterator = std::vector<int>::iterator;
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }

    // const 迭代器
    using const_iterator = std::vector<int>::const_iterator;
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    const_iterator cbegin() const { return data_.cbegin(); }
    const_iterator cend() const { return data_.cend(); }
};
```

---

## 检查工具

### 使用 clang-tidy
```bash
clang-tidy -checks='readability-const-return-type,modernize-use-nullptr' -p build/ src/*.cpp
```

### 使用 cppcheck
```bash
cppcheck --enable=style src/
```

---

## 修复优先级

1. **High**: Formula 访问器返回 const 指针（影响不可变性保证）
2. **Medium**: 添加缺失的 const 成员函数
3. **Low**: 使用类型别名简化返回类型
