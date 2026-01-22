# Code Standards

> 代码质量最佳实践

---

## 错误处理

### 文件 I/O

```cpp
// ✅ 正确：检查读取结果
std::ifstream ltlf(ltlf_file);
if (!ltlf.is_open()) {
    return false;
}

if (!std::getline(ltlf, formula_str)) {
    return false;  // 读取失败（文件为空或 I/O 错误）
}

if (formula_str.empty()) {
    return false;
}
```

### Parser 错误

当前返回错误字符串，可以增强为包含位置信息的结构：

```cpp
struct ParseError {
    std::string message;
    size_t position = 0;
    size_t line = 0;
    size_t column = 0;
};
```

---

## 命名规范

1. **使用完整单词**：避免缩写（如 `rmnext` → `progression`）
2. **使用领域术语**：使用 LTLf 文献中的标准术语
3. **布尔值命名**：使用 `is_`, `has_`, `should_` 前缀
4. **一致性**：相似概念使用相似的命名模式

---

## STL 使用建议

### Hash 组合

```cpp
// 通用的 hash_combine 工具
namespace utils {
    template<typename T>
    inline void hash_combine(std::size_t& seed, const T& value) noexcept {
        constexpr std::size_t golden_ratio = 0x9e3779b9;
        seed ^= std::hash<T>{}(value) + golden_ratio + (seed << 6) + (seed >> 2);
    }
}

// 使用
size_t hash() const noexcept {
    size_t h = 0;
    for (auto f : formulas_) {
        utils::hash_combine(h, f->hash());
    }
    return h;
}
```

### 字符串操作

```cpp
// 使用 std::string_view 避免拷贝
void process(std::string_view str);

// C++20 字符串方法
if (str.starts_with("prefix")) { ... }
```

---

## 字符串格式化

> **注意**：日志输出使用 `LOG_*` 宏（内置 fmt 支持）。这里说的是**非日志输出**场景的字符串格式化。

使用 **spdlog 内置的 fmt 库** (`spdlog/fmt/fmt.h`) 进行字符串格式化：

### 常用场景

| 场景 | 代码示例 | 代码位置 |
|------|---------|----------|
| 错误消息 | `fmt::format("Failed to read: {}", error)` | cosy2.cpp, file_utils.cpp |
| 文件路径 | `fmt::format("{}/bench{}/f{}.ltlf", base, dir, num)` | benchmark.cpp |
| 集合连接 | `fmt::join(items, ", ")` | tableau.cpp, on_the_fly_solver.cpp |
| ID/标签 | `fmt::format("scc_{:03d}", i)` (3位填充) | trace_exporter.cpp |
| 数字格式 | `fmt::format("stage_{:03d}", counter)` | trace_exporter.cpp |

```cpp
#include <spdlog/fmt/fmt.h>

// 错误消息
return tl::unexpected(fmt::format("Cannot open file: {}", filename));

// 文件路径
auto ltlf_file = fmt::format("{}/bench{}/f{}.ltlf", base_dir, bench_dir, bench_num);

// 集合连接（逗号分隔）
auto inputs_str = fmt::format("inputs: [{}]", fmt::join(inputs, ", "));

// 数字填充（3位，不足补0）
auto scc_id = fmt::format("scc_{:03d}", i);  // scc_001, scc_002, ...
```

### 格式化选项

| 选项 | 说明 | 示例 |
|------|------|------|
| `{:d}` | 整数 | `fmt::format("val={:d}", 42)` → `"val=42"` |
| `{:.2f}` | 浮点数(2位小数) | `fmt::format("{:.2f}", 3.14159)` → `"3.14"` |
| `{:03d}` | 整数(3位填充) | `fmt::format("{:03d}", 7)` → `"007"` |
| `{:>10}` | 右对齐(10字符) | `fmt::format("{:>10}", "hi")` → `"        hi"` |

---

## 检查清单

- [ ] 所有文件 I/O 检查返回值
- [ ] 错误消息有足够上下文
- [ ] 命名准确反映语义
- [ ] 使用 STL 或者流行实用的第三方库而非手动实现
- [ ] 热点代码避免不必要的拷贝
