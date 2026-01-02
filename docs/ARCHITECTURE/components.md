# 核心组件设计

> Formula, FormulaPool, TableauState 组件详解

---

## 1. Formula 类

### 1.1 设计原则：不可变性 (Immutable)

```cpp
class Formula {
public:
    enum class OpType {
        True, False, Not, And, Or,
        Next, Until, Release, End, Literal
    };

private:
    OpType op_;              // 操作类型
    Formula* left_;          // 左子公式
    Formula* right_;         // 右子公式
    int var_id_;             // 变量 ID
    size_t hash_;            // 缓存的哈希值

    // 构造函数私有，只能通过 FormulaPool 创建
    Formula(OpType op, Formula* left, Formula* right, int var_id);
};
```

### 1.2 设计优势

| 特性 | 优势 |
|------|------|
| 不可变 | 线程安全，可随意共享 |
| 哈希缓存 | O(1) 结构相等比较 |
| 原始指针 | 无智能指针开销 |
| 私有构造 | 强制通过 Pool 创建 |

### 1.3 内存布局

```
Formula 对象 (24-32 bytes)
├── op_:      1 byte (enum)
├── padding:  3 bytes
├── left_:    8 bytes (ptr)
├── right_:   8 bytes (ptr)
├── var_id_:  4 bytes (int)
└── hash_:    8 bytes (size_t)
```

---

## 2. FormulaPool 类

### 2.1 设计原则：RAII + Hash Consing

```cpp
class FormulaPool {
public:
    // 创建公式 (自动去重)
    Formula* create(OpType op, Formula* left, Formula* right, int var_id);
    Formula* create_true();
    Formula* create_false();
    Formula* create_variable(const std::string& name);

    // 变量管理
    int get_or_create_variable(const std::string& name);
    const std::string& get_variable_name(int id) const;
    bool has_variable(const std::string& name) const;

    ~FormulaPool();  // 自动释放所有 Formula

private:
    std::unordered_set<Formula*> pool_;
    std::vector<std::string> var_names_;
};
```

### 2.2 Hash Consing

相同结构的公式只存储一份：

```cpp
Formula* FormulaPool::create(OpType op, Formula* l, Formula* r, int vid) {
    Formula* f = new Formula(op, l, r, vid);
    auto [it, inserted] = pool_.insert(f);

    if (!inserted) {
        delete f;  // 已存在，删除新建的
        return *it;  // 返回已有的
    }
    return f;
}
```

**效果**:
- `a & a` → 只有一个 `a` 对象
- `(a & b) & (a & b)` → 只有一个 `a & b` 对象

### 2.3 变量管理

```cpp
// 输出优先的变量顺序
std::vector<std::string> get_variable_names() const;

// 自动声明
Formula* f = parser.parse("request");
// pool_.has_variable("request") == true
```

---

## 3. TableauState 类

### 3.1 状态表示

```cpp
class TableauState {
public:
    using FormulaSet = std::unordered_set<Formula*, FormulaHash, FormulaEqual>;

    // 初始状态：包含公式本身及其所有子公式
    static std::unique_ptr<TableauState> initial(Formula* phi, FormulaPool& pool);

    // 状态属性
    bool is_accepting() const;           // 接受状态判定
    bool is_locally_consistent() const;  // 局部一致性

    // 转移函数
    std::unique_ptr<TableauState> next(const Assignment& assignment, FormulaPool& pool) const;

private:
    FormulaSet formulas_;   // 状态中的子公式集合
    size_t hash_;           // 缓存的哈希值
};
```

### 3.2 接受状态条件

状态 Γ 是接受状态 iff:
1. `false ∉ Γ`
2. Γ 是局部一致的
3. 所有 Until 公式 `φ U ψ ∈ Γ` 满足 `ψ ∈ Γ`

### 3.3 局部一致性

Γ 是局部一致的 iff:
- 没有 `φ` 和 `!φ` 同时在 Γ 中
- 对于 `φ & ψ ∈ Γ`，有 `φ ∈ Γ` 且 `ψ ∈ Γ`
- 对于 `φ | ψ ∈ Γ`，有 `φ ∈ Γ` 或 `ψ ∈ Γ`

---

## 4. OnTheFlyGameSolver 类

### 4.1 游戏状态

```cpp
struct GameState {
    TableauState* dfa_state;                             // DFA 状态
    Player player;                                           // System 或 Environment
    std::optional<Assignment> system_chosen_output;        // System 选择的输出（仅 Environment 回合有值）

    // 不变式约定:
    // - player == System 时: system_chosen_output = nullopt
    // - player == Environment 时: system_chosen_output 有值

    bool operator==(const GameState& other) const;
    size_t hash() const;
};
```

### 4.2 核心算法

```cpp
class OnTheFlyGameSolver {
public:
    OnTheFlyGameSolver(Formula* phi, FormulaPool& pool);

    bool is_realizable();

private:
    // Tarjan SCC 分解
    std::vector<std::vector<GameState>> find_sccs();

    // SCC 分类
    std::optional<StateClass> try_classify_scc(const std::vector<GameState>& scc);

    // 向后传播
    bool propagate_classification();
};
```

---

## 5. 组件交互图

```
┌─────────────────────────────────────────────────────────┐
│                      FormulaPool                         │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐   │
│  │Formula* │  │Formula* │  │Formula* │  │Formula* │   │
│  │ true    │──│Formula* │──│Formula* │──│Formula* │   │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘   │
└─────────────────────────────────────────────────────────┘
       │                    │
       ▼                    ▼
┌──────────────────┐  ┌──────────────────┐
│   FormulaParser  │  │  TableauState    │
│  .parse()        │  │  .initial()      │
│  → Formula*      │  │  → TableauState* │
└──────────────────┘  └──────────────────┘
                             │
                             ▼
                   ┌──────────────────────┐
                   │ OnTheFlyGameSolver   │
                   │  .is_realizable()   │
                   │  → bool             │
                   └──────────────────────┘
```

---

## 6. 代码实现细节

### 6.1 unordered_map 初始化参数

在 OnTheFlyDFA 中使用了自定义哈希表的初始化：

```cpp
mutable std::unordered_map<CacheKey, TableauState*, CacheKeyHash, CacheKeyEqual> transition_cache_;
transition_cache_(16, CacheKeyHash{}, CacheKeyEqual{})
```

#### 构造函数签名

```cpp
unordered_map(
    size_type bucket_count,      // 参数 1: 初始桶数量
    const Hash& hash,            // 参数 2: 哈希函数对象
    const KeyEqual& equal,       // 参数 3: 键相等比较函数
    const Allocator& alloc = Allocator()  // 参数 4 (可选)
);
```

#### 参数解释

| 参数 | 值 | 含义 |
|------|-----|------|
| **bucket_count** | `16` | 初始桶数量，影响哈希表容量和重哈希频率 |
| **hash** | `CacheKeyHash{}` | 哈希函数对象（值初始化） |
| **equal** | `CacheKeyEqual{}` | 键相等比较函数对象（值初始化） |

#### 桶数量 (bucket_count)

```cpp
// 桶数量是哈希表中"桶"的初始个数
// 当元素数量 > 桶数量 * load_factor 时，会触发 rehash（扩容）

// 选择 16 的原因：
// - 足够小，不浪费内存
// - 足够大，避免频繁 rehash
// - 16 是 2 的幂，对哈希表性能友好
```

#### 哈希函数和相等比较

```cpp
// CacheKeyHash{} 是"值初始化"的临时对象
// 等价于: CacheKeyHash()

struct CacheKeyHash {
    size_t operator()(const CacheKey& key) const {
        // 计算哈希值
    }
};

struct CacheKeyEqual {
    bool operator()(const CacheKey& a, const CacheKey& b) const {
        // 比较逻辑
    }
};
```

#### `{}` 语法的含义

```cpp
CacheKeyHash{}     // 值初始化 (value-initialization)
                   // 对于没有构造函数参数的类型，等同于默认构造

// 等价写法：
CacheKeyHash{}     ← C++11 统一初始化
CacheKeyHash()     ← C++98 风格（但在某些上下文可能被解析为函数声明）
```

#### 如果不提供这些参数

```cpp
// 默认构造（使用默认参数）
transition_cache_()
// 等价于：
transition_cache_(0, CacheKeyHash{}, CacheKeyEqual{})
//                 ↑
//                 桶数量为 0，第一次插入时会自动扩容

// 默认也是可以的，但显式指定桶数量可以优化性能
```

#### 完整示例

```cpp
struct CacheKey {
    TableauState* state;
    Assignment assignment;
};

struct CacheKeyHash {
    size_t operator()(const CacheKey& key) const {
        size_t h1 = std::hash<TableauState*>{}(key.state);
        size_t h2 = std::hash<Assignment>{}(key.assignment);
        return h1 ^ (h2 << 1);  // 组合哈希
    }
};

struct CacheKeyEqual {
    bool operator()(const CacheKey& a, const CacheKey& b) const {
        return a.state == b.state && a.assignment == b.assignment;
    }
};

// 使用
class OnTheFlyDFA {
    mutable std::unordered_map<CacheKey, TableauState*,
                                CacheKeyHash, CacheKeyEqual> transition_cache_;

    OnTheFlyDFA(Formula* phi, FormulaPool& pool)
        : transition_cache_(16, CacheKeyHash{}, CacheKeyEqual{})
    { }
};
```

**总结**：这是一种性能优化实践——预先指定合理的桶数量，减少哈希表在增长过程中的 rehash 次数。

### 6.2 组合哈希函数 (Boost.Hash)

在 TableauState 中使用了 Boost 风格的组合哈希函数：

```cpp
size_t compute_formula_set_hash(const TableauState::FormulaSet& formulas) {
    size_t h = 0;
    for (formula::Formula* f : formulas) {
        if (f) {
            h ^= f->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
    }
    return h;
}
```

#### 核心原理

| 操作 | 符号 | 作用 |
|------|------|------|
| 异或 | `^` | 结合多个哈希值，可逆、无序 |
| 加法 | `+` | 增加混淆，避免异或的对称性问题 |
| 左移 | `<< 6` | 扩散比特，让低位影响高位 |
| 右移 | `>> 2` | 扩散比特，让高位影响低位 |
| 魔法常数 | `0x9e3779b9` | 黄金比例相关，均匀分布 |

#### 魔法常数 `0x9e3779b9`

```cpp
0x9e3779b9 = 2654435769 (十进制)
// 来源：2^32 / φ，其中 φ = (1 + √5) / 2 ≈ 1.618 (黄金比例)
```

这个常数广泛应用于：
- Knuth 的乘法哈希
- 线性同余生成器
- C++ 标准库的 `std::hash` 组合函数

#### 逐行解析

```cpp
h ^= f->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
│   │           │              │        │
│   │           │              │        └─ 右移 2 位（高位扩散到低位）
│   │           │              └────────── 左移 6 位（低位扩散到高位）
│   │           └───────────────────────── 魔法常数（增加随机性）
│   └───────────────────────────────────── 异或：结合新元素的哈希
└────────────────────────────────────────── 累积哈希值
```

#### 如果只用异或会怎样？

```cpp
// ❌ 糟糕的哈希函数
size_t bad_hash(const FormulaSet& formulas) {
    size_t h = 0;
    for (Formula* f : formulas) {
        h ^= f->hash();
    }
    return h;
}

// 问题：
// {A, B, C}  →  hashA ^ hashB ^ hashC
// {C, B, A}  →  hashC ^ hashB ^ hashA  = 相同！(顺序敏感丢失)
// {A, A}     →  hashA ^ hashA = 0
// {B, B}     →  hashB ^ hashB = 0  = 相同！(重复问题)
```

#### Boost 风格哈希的优势

```cpp
// ✅ 好的哈希函数
h ^= f->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);

// 优势：
// 1. 顺序敏感：{A, B} ≠ {B, A}
// 2. 重复敏感：{A, A} ≠ {B, B}
// 3. 比特扩散：小的变化 → 大的哈希变化
```

#### 碰撞概率

对于 64 位 size_t：

| 场景 | 碰撞概率 |
|------|---------|
| 随机输入，2^32 个不同状态 | ≈ 0.00000006% |
| 随机输入，2^24 个不同状态 (~1600万) | ≈ 0.04% |
| 随机输入，10^6 个不同状态 | ≈ 0.000000003% |

**结论**：这是一个**工业级、经过验证**的哈希函数（Boost 和标准库都在用），碰撞概率在实际应用中可以忽略。即使碰撞，`unordered_set` 还会用相等比较做二次检查，只影响性能不影响正确性。

#### 算法来源

这是 **Boost.Hash** 的标准组合哈希算法，广泛用于：
- C++ 标准库的 `std::hash<std::pair>` 和 `std::hash<std::tuple>`
- Boost 库的 `boost::hash_combine`
- 各种哈希表实现

#### 现代替代方案

```cpp
// C++14 及以上可以使用标准化的组合
#include <functional>

size_t compute_formula_set_hash(const FormulaSet& formulas) {
    size_t h = 0;
    for (Formula* f : formulas) {
        h ^= std::hash<Formula*>{}(f) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}
```
