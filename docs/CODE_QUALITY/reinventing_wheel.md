# 重复造轮子分析

**原则**: 优先使用 STL 和现有库，除非有明确的性能或功能需求需要自定义实现

---

## Medium 问题

### 1. 手动实现的 hash 函数

**位置**: `include/automata/dfa.hpp:86-92`

```cpp
size_t hash() const {
    size_t h = 0;
    for (auto f : formulas_) {
        h ^= f->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}
```

**问题描述**:
- 这是类似于 `std::hash_combine` 的手动实现
- 使用了魔数 `0x9e3779b9` 但没有注释说明
- 没有使用标准库的 hash 工具

**更好的实现**:
```cpp
// 选项 A: 使用标准库的 hash combine (C++26)
// 当前需要手动实现，但可以添加注释

// 选项 B: 使用明确命名的辅助函数
namespace detail {
    inline void hash_combine(std::size_t& seed, std::size_t value) {
        // 0x9e3779b9 is derived from the golden ratio: 2^32 / φ
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}

size_t hash() const {
    size_t h = 0;
    for (auto f : formulas_) {
        detail::hash_combine(h, f->hash());
    }
    return h;
}

// 选项 C: 实现标准 hash 特化（如果需要在 unordered_container 中使用）
namespace std {
    template<> struct hash<PrimitiveFormula> {
        size_t operator()(const PrimitiveFormula& pf) const noexcept {
            return pf.hash();
        }
    };
}
```

---

### 2. 手动实现的 unordered_set hash

**位置**: `include/automata/dfa.hpp:144-152`

```cpp
size_t hash() const {
    size_t h = 0;
    for (int v : positive_vars) {
        h ^= std::hash<int>{}(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    for (int v : negative_vars) {
        h ^= std::hash<int>{}(-v) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}
```

**问题描述**:
- 对集合元素手动组合 hash
- 可以直接使用 `std::hash<std::unordered_set<int>>`（如果支持）

**更好的实现**:
```cpp
size_t hash() const noexcept {
    // Combine the hashes of both sets using hash_combine
    size_t h = 0;
    for (int v : positive_vars) {
        detail::hash_combine(h, std::hash<int>{}(v));
    }
    for (int v : negative_vars) {
        detail::hash_combine(h, std::hash<int>{}(-v));
    }
    return h;
}
```

---

## 可用 STL 设施总结

### Hash 相关
- `std::hash<T>` - 标准哈希函数
- `std::hash_combine` (C++26) 或 Boost 实现
- `std::unordered_map`, `std::unordered_set`

### 字符串处理
- `std::string_view` (C++17) - 避免拷贝
- `std::string::starts_with`, `std::string::ends_with` (C++20)
- `std::format` (C++20) - 替代字符串拼接

### 算法
- `std::clamp` (C++17) - 值限制
- `std::reduce` (C++17) - 并行求和
- `std::ranges` (C++20) - 范围算法

### 可选值
- `std::optional<T>` (C++17) - 可能空的值
- `std::variant<Ts...>` (C++17) - 类型联合
- `std::any` (C++17) - 类型擦除容器

---

## 检查清单

- [ ] 字符串操作是否可以使用 `std::string_view` 避免拷贝？
- [ ] Hash 函数是否可以利用标准库？
- [ ] 容器操作是否可以使用 `std::ranges` 简化？
- [ ] 可选值是否使用 `std::optional` 而非指针？
- [ ] 类型联合是否可以使用 `std::variant`？

---

## 重构建议

### 当前代码模式
```cpp
size_t hash() const {
    size_t h = 0;
    for (auto f : formulas_) {
        h ^= f->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}
```

### 推荐模式
```cpp
// 1. 创建通用的 hash_combine 工具
namespace utils {
    template<typename T>
    inline void hash_combine(std::size_t& seed, const T& value) noexcept {
        // Golden ratio constant for better hash distribution
        constexpr std::size_t golden_ratio = 0x9e3779b9;
        seed ^= std::hash<T>{}(value) + golden_ratio + (seed << 6) + (seed >> 2);
    }

    template<typename It>
    inline void hash_range(std::size_t& seed, It first, It last) noexcept {
        for (; first != last; ++first) {
            hash_combine(seed, *first);
        }
    }
}

// 2. 使用工具函数
size_t hash() const noexcept {
    size_t h = 0;
    utils::hash_range(h, formulas_.begin(), formulas_.end());
    return h;
}
```

### 优势
- 单一真实来源
- 更容易测试
- 更容易维护
- 添加注释说明魔数来源
