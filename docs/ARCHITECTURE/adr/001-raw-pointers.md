# ADR-001: 为什么使用原始指针而非 shared_ptr?

## 状态
有效

## 上下文
Formula 对象之间有树状引用关系（左子、右子）。需要决定使用何种指针类型来管理这些关系。

## 决策
使用 **原始指针**，由 FormulaPool 统一管理所有权。

## 理由

| 方面 | shared_ptr | 原始指针 + Pool |
|------|-----------|----------------|
| 性能 | 引用计数开销 (原子操作) | 无开销 |
| 内存 | 每个指针 +8 bytes | 正常指针大小 |
| 线程安全 | 是，但不需要 | Formula 不可变，天然安全 |
| 生命周期 | 自动 | FormulaPool RAII 保证 |
| 循环引用 | 需要 weak_ptr | 不存在 (DAG) |

**关键点**:
- Formula 是不可变的，一旦创建不会修改
- FormulaPool 拥有所有 Formula，析构时统一释放
- 引用关系不会形成环 (树状结构)
- Hash consing 保证没有重复对象

## 代码示例

```cpp
class FormulaPool {
public:
    ~FormulaPool() {
        for (Formula* f : pool_) {
            delete f;  // 统一释放
        }
    }

    Formula* create(OpType op, Formula* left, Formula* right) {
        Formula* f = new Formula(op, left, right);
        auto [it, inserted] = pool_.insert(f);
        if (!inserted) delete f;
        return *it;
    }

private:
    std::unordered_set<Formula*> pool_;
};
```

## 后果

**正面**:
- 性能更优，无引用计数开销
- 内存占用更小
- 代码简洁，无需处理 weak_ptr

**负面**:
- 需要保证 Formula 生命周期正确
- 不能随意传递 FormulaPool 副本

**风险**:
- 如果 FormulaPool 析构时仍有 Formula 裸指针被引用，会悬空
- **缓解**: FormulaPool 通常是单例或长生命周期对象

## 替代方案

### 方案 A: shared_ptr
- ❌ 性能开销
- ❌ 代码复杂 (需要 enable_shared_from_this)

### 方案 B: unique_ptr
- ❌ 无法共享 (多个父节点引用同一子节点)

## 相关决策
- ADR-002: Tableau 选择
- ADR-003: 显式状态优先
