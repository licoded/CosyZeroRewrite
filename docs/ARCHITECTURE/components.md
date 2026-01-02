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
    TableauState* dfa_state;     // DFA 状态
    Player player;               // System 或 Environment
    Assignment current_output;   // 当前输出赋值

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
