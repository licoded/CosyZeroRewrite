# 传播逻辑分析笔记

**日期**: 2026-01-02 PM
**任务**: 分析 LTLf Synthesis 传播逻辑，核对文档与代码实现差异
**状态**: 进行中

---

## 1. 核心发现总结

### 1.1 传播规则本身是正确的

| 玩家 | 规则 | 文档描述 | 代码实现 | 状态 |
|------|------|----------|----------|------|
| **System** | 任一后继 Swin → Swin | ✅ | ✅ | 正确 |
| **System** | 所有后继 Ewin → Ewin | ✅ | ✅ | 正确 |
| **Environment** | 任一后继 Ewin → Ewin | ✅ | ✅ | 正确 |
| **Environment** | 所有后继 Swin → Swin | ✅ | ✅ | 正确 |

**结论**: `propagate_classification()` 函数中的传播逻辑实现符合文档规范。

### 1.2 真正的问题：执行时机

问题不在传播规则本身，而在于**什么时候执行传播**：

```
当前算法流程：
┌─────────────────────────────────────────────────────────────┐
│ while (worklist 不空 && 初始状态未分类) {                    │
│     1. Pop 一个状态                                         │
│     2. expand_state(state)      ← 展开一个状态              │
│     3. find_sccs()              ← 立即做 SCC 分解！         │
│     4. classify_scc()           ← 立即分类 SCC！            │
│     5. propagate_classification() ← 立即传播！              │
│     6. 检查初始状态是否已分类 → 如果是就退出！              │
│ }                                                            │
└─────────────────────────────────────────────────────────────┘
```

**问题**：
- 每展开一个状态就立即做 SCC 分解
- 此时大部分后继状态可能还没展开
- SCC 不完整 → 分类不充分 → 传播不完整
- 如果初始状态被错误分类，算法过早退出

---

## 2. 文档 vs 代码 详细对比

### 2.1 文档描述 (on_the_fly_algorithm.md)

**循环条件** (line 269):
```cpp
while (!worklist_.empty() && !is_initial_classified()) {
```

**传播规则** (line 221-227):
| 当前状态 | 条件 | 结果 |
|---------|------|------|
| System | **任一**后继是 Swin | Swin |
| System | **所有**后继是 Ewin | Ewin |
| Environment | **所有**后继是 Swin | Swin |
| Environment | **任一**后继是 Ewin | Ewin |

### 2.2 代码实现 (on_the_fly_solver.cpp)

**循环条件** (line 92):
```cpp
while (!worklist_.empty() && !is_initial_classified()) {
```

**propagate_classification()** (line 517-580):
- System: `has_swin → Swin`, `all_ewin → Ewin` ✅
- Environment: `has_ewin → Ewin`, `all_swin → Swin` ✅

**classify_scc()** (line 450-482):
- System: `has_swin_succ → Swin` ✅
- Environment: `all_swin_succ → Swin` ✅

### 2.3 对比结论

✅ **传播规则完全一致** - 代码实现符合文档描述
⚠️ **执行策略是问题所在** - 见下一节

---

## 3. 算法执行策略分析

### 3.1 当前实现的问题

```
时间线：
t0: worklist = [initial]
t1: expand(initial) → successors = [s1, s2, s3]
t2: find_sccs() → 只找到 {initial} 的 SCC（因为 s1,s2,s3 还没展开）
t3: classify_scc → initial 被分类（可能基于不完整信息）
t4: propagate → 初始状态已分类 → 退出循环！
```

**问题场景示例**：

```
初始状态 s0 的后继是 {s1, s2}
- s1 还没展开，不知道它的后继是什么
- s2 还没展开，不知道它的后继是什么
- 如果 s0 被提前分类为 Swin（因为某个不完整的判断）
- 算法就退出了，但实际上 s1,s2 可能都导致 Ewin
```

### 3.2 正确的算法应该是

```
时间线：
t0: worklist = [initial]
t1: expand(initial) → successors = [s1, s2, s3]
t2: 添加 s1,s2,s3 到 worklist
t3: expand(s1) → ...
t4: expand(s2) → ...
... (展开所有可达状态)
tN: worklist 为空，所有状态都已展开
tN+1: find_sccs() → 在完整图上找 SCC
tN+2: classify_scc() → 基于完整信息分类
tN+3: propagate → 完整传播
```

### 3.3 建议修改策略

**策略 1**: 先展开所有状态，再执行 SCC 和传播

```cpp
// Phase 1: Expand all states
while (!worklist_.empty()) {
    GameState state = worklist_.back();
    worklist_.pop_back();
    if (expanded_.count(state)) continue;
    expand_state(state);

    // Add all successors to worklist
    for (const auto& succ : successors_[state]) {
        if (!expanded_.count(succ)) {
            worklist_.push_back(succ);
        }
    }
}

// Phase 2: SCC decomposition on complete graph
auto sccs = find_sccs();

// Phase 3: Classify SCCs
for (const auto& scc : sccs) {
    classify_scc(scc);
}

// Phase 4: Propagate (multiple iterations to fixed point)
while (propagate_classification()) {
    // Continue until no more changes
}
```

**策略 2**: 延迟检查 `is_initial_classified()`

```cpp
while (!worklist_.empty()) {
    // ... expand and classify ...
    // 不在循环中间检查初始状态
}

// 只在所有状态都展开后才检查
if (!is_initial_classified()) {
    // 进入 terminal classification
}
```

---

## 4. is_initial_classified() 的隐患

### 4.1 代码位置

**主循环** (line 92):
```cpp
while (!worklist_.empty() && !is_initial_classified()) {
```

**SCC 处理后** (line 134-138):
```cpp
bool initial_done = propagate_classification();
if (initial_done) {
    LOG_DEBUG("OnTheFlyGameSolver: initial state classified!");
    break;  // ← 直接退出！
}
```

### 4.2 问题分析

| 场景 | 描述 | 风险 |
|------|------|------|
| 过早分类 | 初始状态基于不完整信息被分类 | 错误结果 |
| 无法修正 | 一旦分类就退出，没有重新评估的机会 | 错误固化 |
| 传播不完整 | 后续状态可能改变初始状态的分类 | 但已经退出了 |

### 4.3 调试建议

添加环境变量 `COSY_DISABLE_EARLY_EXIT=1` 来禁用提前退出：

```cpp
const char* disable_early_exit = std::getenv("COSY_DISABLE_EARLY_EXIT");
bool debug_no_early_exit = (disable_early_exit &&
                            std::string(disable_early_exit) == "1");

while (!worklist_.empty() && (debug_no_early_exit || !is_initial_classified())) {
    // ...
}
```

---

## 5. 添加的一致性检查功能

### 5.1 新增方法

```cpp
size_t OnTheFlyGameSolver::check_propagation_consistency() const;
```

### 5.2 检查规则

| 状态 | 分类 | 应该满足的条件 |
|------|------|---------------|
| System | Swin | 至少有一个后继是 Swin |
| System | Ewin | 所有后继都是 Ewin |
| Environment | Swin | 所有后继都是 Swin |
| Environment | Ewin | 至少有一个后继是 Ewin |

### 5.3 使用方法

```bash
# 启用传播一致性检查
COSY_DEBUG_PROPAGATION=1 ./build/Cosy2 -f examples/test.ltlf -p examples/test.part
```

### 5.4 输出示例

```
[WARN] Propagation violation: System state {dfa=xxx, player=Sys} is Swin but has no Swin successor (has 2 successors)
[ERROR] Found 3 propagation consistency violations!
```

---

## 6. 下一步工作计划

### 6.1 立即可做

- [x] 添加传播一致性检查方法
- [x] 添加调试模式禁用提前退出
- [ ] 编译并测试新功能
- [ ] 在 benchmark 上运行一致性检查

### 6.2 需要进一步分析

- [ ] 修改 SCC 查找策略：先展开所有状态
- [ ] 分析哪些公式受提前退出影响最大
- [ ] 与 Cosy 参考实现对比结果

### 6.3 长期改进

- [ ] 考虑实现真正的 on-the-fly 算法（带剪枝）
- [ ] 添加策略提取功能验证分类结果
- [ ] 性能分析和优化

---

## 7. 代码更改摘要

### 7.1 头文件 (on_the_fly_solver.hpp)

添加了新方法声明：
```cpp
size_t check_propagation_consistency() const;
```

### 7.2 实现文件 (on_the_fly_solver.cpp)

1. **调试模式变量** (line 86-93):
   ```cpp
   const char* disable_early_exit = std::getenv("COSY_DISABLE_EARLY_EXIT");
   bool debug_no_early_exit = ...
   ```

2. **修改循环条件** (line 101):
   ```cpp
   while (!worklist_.empty() && (debug_no_early_exit || !is_initial_classified()))
   ```

3. **添加一致性检查** (line 215-224):
   ```cpp
   const char* debug_prop = std::getenv("COSY_DEBUG_PROPAGATION");
   if (debug_prop && std::string(debug_prop) == "1") {
       size_t violations = check_propagation_consistency();
   }
   ```

4. **实现一致性检查方法** (line 596-691):
   ```cpp
   size_t OnTheFlyGameSolver::check_propagation_consistency() const {
       // 检查所有已分类状态的传播逻辑一致性
   }
   ```

---

## 8. 测试命令

```bash
# 正常运行
./build/Cosy2 -f examples/test.ltlf -p examples/test.part

# 禁用提前退出（展开所有状态）
COSY_DISABLE_EARLY_EXIT=1 ./build/Cosy2 -f examples/test.ltlf -p examples/test.part

# 启用传播一致性检查
COSY_DEBUG_PROPAGATION=1 ./build/Cosy2 -f examples/test.ltlf -p examples/test.part

# 同时启用两个调试选项
COSY_DISABLE_EARLY_EXIT=1 COSY_DEBUG_PROPAGATION=1 ./build/Cosy2 -f examples/test.ltlf -p examples/test.part

# Benchmark 测试
./build/benchmark_runner benchmarks/sm1000 1 50
```

---

## 9. 参考资料

- `docs/ARCHITECTURE/on_the_fly_algorithm.md` - 算法详细描述
- `docs/ARCHITECTURE/synthesis.md` - Synthesis 模块架构
- `docs/BUGS/open.md` - 已知 Bug 列表
- `src/synthesis/on_the_fly_solver.cpp` - 主要实现文件
- `include/synthesis/on_the_fly_solver.hpp` - 头文件

---

**最后更新**: 2026-01-02 PM
