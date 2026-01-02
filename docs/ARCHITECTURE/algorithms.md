# 算法复杂度分析

> 各算法的时间和空间复杂度

---

## 1. 转换操作

### 1.1 NNF (否定范式)

**算法**: 递归应用德摩根定律，将否定推向叶子

```cpp
!(φ & ψ) → !φ | !ψ
!(φ | ψ) → !φ & !ψ
!!φ → φ
!X(φ) → X(!φ)
```

**复杂度**: O(n)
- 时间: 单次遍历公式树
- 空间: 递归深度 = 公式深度

### 1.2 Simplify (化简)

**算法**: 局部重写规则

```cpp
φ & true → φ
φ | false → φ
φ & φ → φ
φ & !φ → false
!(true) → false
!(false) → true
```

**复杂度**: O(n)
- 时间: 固定规则集，每次重写常数时间
- 空间: O(1) 额外空间

### 1.3 XNF (Next 正规型)

**算法**: 递推 Next 算子到叶子

```cpp
X(φ & ψ) → X(φ) & X(ψ)
X(φ | ψ) → X(φ) | X(ψ)
X(!φ) → !X(φ)
X(X(φ)) → X(X(φ))
```

**复杂度**: O(n)
- 时间: 单次遍历
- 空间: 递归深度

### 1.4 rmnext (消除 Next)

**算法**: 引入新变量替换 Next

```cpp
X(φ) → v_next   (where v_next = next(φ))
```

**复杂度**: O(n)
- 时间: 与 Next 算子数量线性相关

---

## 2. Tableau 构造

### 2.1 状态空间

**最坏情况**: O(2^n) 状态
- n = 原始公式的子公式数量
- 每个子公式可以存在或不存在

**典型情况**: O(n) 状态
- Hash consing 和局部一致性大幅减少状态数

### 2.2 转移计算

**复杂度**: O(|Γ| × 2^|V|)
- |Γ| = 状态中的子公式数
- |V| = 变量数
- 每个转移尝试所有可能的赋值

---

## 3. 游戏求解

### 3.1 游戏图大小

**状态数**: O(2^n × 2^|V_out|)
- n = DFA 状态数
- |V_out| = 输出变量数

**转移数**: O(|S| × 2^|V_in|)
- |S| = 游戏状态数
- |V_in| = 输入变量数

### 3.2 Tarjan SCC

**复杂度**: O(V + E)
- V = 游戏状态数
- E = 转移数

**空间**: O(V)
- 索引数组、栈、低链接值

### 3.3 状态分类

**复杂度**: O(V)
- 每个 SCC 只处理一次

### 3.4 向后传播

**复杂度**: O(E)
- 每条边处理一次

---

## 4. 总体复杂度

| 阶段 | 时间复杂度 | 空间复杂度 |
|------|-----------|-----------|
| NNF | O(n) | O(depth) |
| Simplify | O(n) | O(1) |
| XNF | O(n) | O(depth) |
| Tableau 构建 | O(2^n × 2^|V|) | O(2^n) |
| SCC 分解 | O(V+E) | O(V) |
| 状态分类 | O(V) | O(V) |
| 向后传播 | O(E) | O(V) |

**瓶颈**: Tableau 状态构造 (指数)

---

## 4. BMC (Bounded Model Checking) 实现

### 4.1 增量检查策略

**算法**: 时间展开 + 指数增量

```
bound = 2
while (not proved && bound < max_bound) {
    check_with_bound(bound)
    if (timeout) break
    bound *= 2  # 指数增长: 2, 4, 8, 16, ...
}
```

**参数配置**:
- **初始边界**: 2
- **增长因子**: 2x (指数)
- **默认乘数**: 8x (auto-detected bound × 8)
- **单步超时**: min(timeout_ms / 4, 30000ms)

### 4.2 时间估算

**模型**: 幂律拟合 `time = c * bound^k`

```cpp
// 拟合参数
double k = estimate_exponent(sample_times);  // 通常 k ≈ 1.5-2.5
double c = fit_constant(sample_times, k);

// 预测
double predicted_time = c * pow(target_bound, k);
```

**用途**: 提前预估复杂公式的检查时间

### 4.3 超时处理

| 场景 | 处理方式 |
|------|---------|
| 单步超时 | 记录 PASS，日志记录详细 |
| 整体超时 | 终止，返回 INCONCLUSIVE |
| 超时日志 | `logs/timeouts_YYYYMMDD_HHMMSS.csv` |

**超时日志格式**:
```csv
timestamp,formula1,formula2,bound,timeout_ms,elapsed_ms,result
2026-01-02T02:30:00,"X(X(a U b))","X(X(a U b))",32,30000,1523,TIMEOUT
```

---

## 5. 优化方向

### 5.1 BDD 符号化

**目标**: 将显式状态改为 BDD 表示

| 方面 | 显式状态 | BDD 符号化 |
|------|---------|-----------|
| 状态存储 | O(2^n) | O(n × 2^k) |
| 转移计算 | 逐状态 | 符号操作 |
| 适用规模 | n ≤ 15 | n ≤ 100 |

### 5.2 On-the-Fly

**优势**: 懒构造 + 早终止

- 只扩展需要的状态
- 发现 Ewin 立即返回
- 平均情况显著加速

### 5.3 缓存优化

- 转移函数结果缓存
- 状态哈希去重
- SCC 结果复用
