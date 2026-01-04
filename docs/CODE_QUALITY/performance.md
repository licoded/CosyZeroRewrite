# 性能问题分析

---

## Medium 问题

### 1. 优先级查找使用 switch 而非查表

**位置**: `src/formula/formula.cpp:31-51`

```cpp
static int get_precedence(Formula::OpType op) {
    switch (op) {
        case Formula::OpType::Or: return 1;
        case Formula::OpType::And: return 2;
        case Formula::OpType::Until:
        case Formula::OpType::Release: return 3;
        case Formula::OpType::Not:
        case Formula::OpType::Next: return 4;
        case Formula::OpType::Literal:
        case Formula::OpType::True:
        case Formula::OpType::False: return 5;
        default: return 0;
    }
}
```

**问题描述**:
- 每次 `to_string()` 调用都会执行这个 switch
- OpType 是枚举，可以数组索引直接访问
- 当前实现是 O(n) 而非 O(1)

**优化方案**:
```cpp
// 在类中定义为静态 constexpr 数组
class Formula {
private:
    static constexpr int PRECEDENCE[] = {
        0,  // None (index 0)
        4,  // Not (index 1)
        2,  // And (index 2)
        1,  // Or (index 3)
        3,  // Until (index 4)
        3,  // Release (index 5)
        4,  // Next (index 6)
        5,  // Literal (index 7)
        5,  // True (index 8)
        5,  // False (index 9)
    };

    static int get_precedence(OpType op) noexcept {
        return PRECEDENCE[static_cast<int>(op)];
    }
};
```

**性能收益**:
- 编译时常量，零运行时开销
- O(1) 访问
- 更好的分支预测

---

## Low 问题

### 2. 括号检查可以提前返回

**位置**: `src/formula/formula.cpp:11-28`

```cpp
static bool is_wrapped_in_parens(const std::string& s) {
    if (s.empty()) return false;
    if (s[0] != '(') return false;
    if (s.back() != ')') return false;

    // Check that the closing paren matches the opening one
    int depth = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '(') depth++;
        else if (s[i] == ')') depth--;
        if (depth == 0 && i < s.size() - 1) {
            return false;
        }
    }
    return depth == 0;
}
```

**问题描述**:
- 总是遍历整个字符串
- 可以在发现不匹配时立即返回

**优化方案**:
```cpp
static bool is_wrapped_in_parens(const std::string& s) {
    if (s.size() < 2) return false;
    if (s[0] != '(' || s.back() != ')') return false;

    // Check that the closing paren matches the opening one
    int depth = 0;
    for (size_t i = 0; i < s.size() - 1; ++i) {  // Note: s.size() - 1
        if (s[i] == '(') depth++;
        else if (s[i] == ')') {
            depth--;
            if (depth == 0) {
                return false;  // Early exit - closing paren before end
            }
        }
    }
    return depth == 1;  // Should be 1 at the end (unclosed opening paren)
}
```

---

## 其他性能考虑

### 字符串拷贝

**当前模式**:
```cpp
std::string result = base_dir + "/bench" + std::to_string(dir) + "/f" + std::to_string(bench_num) + ".ltlf";
```

**优化建议**:
```cpp
// 使用 std::string::reserve 预分配
std::string result;
result.reserve(base_dir.size() + 20);  // 预估大小
result += base_dir;
result += "/bench";
result += std::to_string(dir);
// ...

// 或使用 std::ostringstream (多次拼接时)
std::ostringstream oss;
oss << base_dir << "/bench" << dir << "/f" << bench_num << ".ltlf";
auto result = oss.str();
```

### 引用传递

**当前模式**:
```cpp
void process(std::string str);  // 按值传递
```

**优化建议**:
```cpp
void process(const std::string& str);  // 按引用传递
// 对于只读参数
void process(std::string_view str);   // C++17，避免拷贝
```

### 移动语义

**当前模式**:
```cpp
std::string get_string() {
    std::string result = "something";
    return result;  // 可能触发拷贝（取决于 RVO）
}
```

**优化建议**:
```cpp
std::string get_string() {
    return "something";  // 直接返回，RVO 优化
}

// 对于复杂对象
std::vector<int> get_vector() {
    std::vector<int> result;
    // ...
    return result;  // C++17 保证 RVO/move
}
```

---

## 性能分析工具

### Valgrind / Callgrind
```bash
valgrind --tool=callgrind ./your_program
callgrind_annotate callgrind.out.<pid>
```

### gprof
```bash
gcc -pg -o program program.cpp
./program
gprof program gmon.out > analysis.txt
```

### perf (Linux)
```bash
perf record -g ./your_program
perf report
```

### Visual Studio Profiler (Windows)
内置的性能分析工具

---

## 检查清单

- [ ] 热点循环是否优化？
- [ ] 是否有不必要的字符串拷贝？
- [ ] 是否可以使用 `std::string_view`？
- [ ] 查找表是否可以用数组替代 switch？
- [ ] 是否正确使用移动语义？
- [ ] 是否预分配容器容量？
- [ ] 是否使用 `constexpr` 和 `noexcept` 帮助优化？
