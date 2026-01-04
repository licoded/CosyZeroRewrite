# 错误处理分析

---

## Medium 问题

### 1. Parser 错误缺少位置信息

**位置**: `include/formula/formula_parser.hpp:62-68`

```cpp
const std::string& error() const { return error_; }
bool has_error() const { return !error_.empty(); }
```

**问题描述**:
- Parser 只返回错误字符串，没有行列号信息
- 调用者无法准确定位错误位置
- 调试复杂公式时很困难

**当前问题示例**:
```
Error: Unexpected token
```

**建议增强**:
```cpp
struct ParseError {
    std::string message;
    size_t line = 0;
    size_t column = 0;
    size_t position = 0;  // 在输入字符串中的位置
};

class FormulaParser {
    ParseError error_;  // 替代 error_

public:
    const ParseError& error() const { return error_; }
    bool has_error() const { return !error_.message.empty(); }

    // 格式化错误信息
    std::string error_message() const {
        if (!has_error()) return "";
        std::ostringstream oss;
        oss << "Line " << error_.line << ", Column " << error_.column
            << ": " << error_.message;
        return oss.str();
    }
};
```

---

### 2. 文件读取后未检查 getline 结果

**位置**: `src/synthesis/synthesis.cpp:110-125`

```cpp
// Read formula
std::ifstream ltlf(ltlf_file);
if (!ltlf.is_open()) {
    return false;
}

std::getline(ltlf, formula_str);
// ⚠️ 没有检查 getline 是否成功
```

**问题描述**:
- 文件打开成功不代表读取成功
- 空文件会导致 `formula_str` 保持空
- 后续代码可能崩溃

**建议修复**:
```cpp
std::ifstream ltlf(ltlf_file);
if (!ltlf.is_open()) {
    return false;
}

if (!std::getline(ltlf, formula_str)) {
    // 读取失败（文件为空或 I/O 错误）
    return false;
}

// 检查是否为空行
if (formula_str.empty()) {
    return false;
}
```

---

## 错误处理模式

### 模式 1: 返回 bool + 错误消息
```cpp
bool read_config(const std::string& path, std::string& error) {
    // 失败时设置 error 并返回 false
    if (!open_file(path)) {
        error = "Cannot open file: " + path;
        return false;
    }
    return true;
}

// 使用
std::string err;
if (!read_config("config.txt", err)) {
    std::cerr << "Error: " << err << std::endl;
}
```

### 模式 2: std::optional (C++17)
```cpp
std::optional<Config> read_config(const std::string& path) {
    if (!open_file(path)) {
        return std::nullopt;
    }
    return Config{...};
}

// 使用
auto config = read_config("config.txt");
if (!config) {
    std::cerr << "Failed to read config" << std::endl;
}
```

### 模式 3: std::expected (C++23) 或 Boost
```cpp
// 需要外部库，但提供最好的错误处理
```

### 模式 4: 异常（对于可恢复错误不推荐）
```cpp
// 只用于真正的"异常"情况
```

---

## 本项目的建议

### 对于文件 I/O
```cpp
// 返回 bool + 错误消息的引用
bool read_benchmark(..., std::string& error_msg) {
    std::ifstream file(path);
    if (!file) {
        error_msg = "Cannot open file: " + path;
        return false;
    }
    // ...
    return true;
}
```

### 对于 Parser
```cpp
// 增强错误结构
struct ParseError {
    std::string message;
    size_t position = 0;
    size_t line = 0;
    size_t column = 0;
    enum class Type { UnexpectedToken, InvalidSyntax, IncompleteInput } type;
};
```

### 对于 Synthesis
```cpp
// 使用 std::optional 表示可能失败的操作
std::optional<bool> is_realizable(Formula* f);
// std::nullopt = 未知/错误
// true = 可实现
// false = 不可实现
```

---

## 错误处理最佳实践

### 1. 区分不同类型的错误
```cpp
enum class ErrorCode {
    Success,
    FileNotFound,
    ParseError,
    RuntimeError,
    Timeout
};
```

### 2. 提供上下文信息
```cpp
// ✗ 不够信息
return false;

// ✓ 更好的信息
error_msg = "Cannot open " + path + ": " + strerror(errno);
return false;
```

### 3. 错误消息的一致性
```cpp
// 统一格式
[模块名] 错误类型: 详细描述
// 例如:
[Parser] Syntax Error at line 5: Unexpected token ')'
```

### 4. 日志记录
```cpp
// 在错误发生时记录日志
if (!read_file(path)) {
    LOG_ERROR("Failed to read file: " << path);
    return false;
}
```

---

## 检查清单

- [ ] 所有文件 I/O 是否检查返回值？
- [ ] Parser 错误是否包含位置信息？
- [ ] 错误消息是否有足够上下文？
- [ ] 是否区分了可恢复和不可恢复错误？
- [ ] 错误处理模式是否一致？
