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

## 检查清单

- [ ] 所有文件 I/O 检查返回值
- [ ] 错误消息有足够上下文
- [ ] 命名准确反映语义
- [ ] 使用 STL 设施而非手动实现
- [ ] 热点代码避免不必要的拷贝
